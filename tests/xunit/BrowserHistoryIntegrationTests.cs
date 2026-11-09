using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using Xunit;

namespace BrowserHistory.XunitTests;

/// <summary>
/// Actual xUnit.net v2 tests written in C#. Each test drives the compiled C++
/// console program as a black box. C++ implementation remains the production code.
/// Build C++ first. Set BROWSER_HISTORY_EXE if the automatic build-path search fails.
/// </summary>
public sealed class BrowserHistoryIntegrationTests
{
    private static string LocateExecutable()
    {
        string? configured = Environment.GetEnvironmentVariable("BROWSER_HISTORY_EXE");
        if (!string.IsNullOrWhiteSpace(configured))
        {
            if (!File.Exists(configured)) throw new FileNotFoundException("BROWSER_HISTORY_EXE not found", configured);
            return Path.GetFullPath(configured);
        }
        var baseDir = new DirectoryInfo(AppContext.BaseDirectory);
        for (DirectoryInfo? folder = baseDir; folder != null; folder = folder.Parent)
        {
            foreach (var relative in new[] {
                "build/browser_history", "build/browser_history.exe",
                "build/Release/browser_history.exe", "build/Debug/browser_history.exe"
            })
            {
                var candidate = Path.Combine(folder.FullName, relative);
                if (File.Exists(candidate)) return candidate;
            }
        }
        throw new FileNotFoundException("Build the C++ program first, or set BROWSER_HISTORY_EXE to its absolute path.");
    }

    private static string Run(params string[] commands)
    {
        using var process = new Process
        {
            StartInfo = new ProcessStartInfo
            {
                FileName = LocateExecutable(),
                Arguments = "https://example.com",
                RedirectStandardInput = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            }
        };
        process.Start();
        process.StandardInput.WriteLine(string.Join(Environment.NewLine, commands.Append("0")));
        process.StandardInput.Close();
        string output = process.StandardOutput.ReadToEnd();
        string error = process.StandardError.ReadToEnd();
        Assert.True(process.WaitForExit(10000), "C++ program did not terminate");
        Assert.Equal(0, process.ExitCode);
        Assert.True(string.IsNullOrWhiteSpace(error), error);
        return output;
    }

    [Fact]
    public void InitialHomepageIsActive()
    {
        string output = Run("4");
        Assert.Contains("Current: #1 | URL: https://example.com", output);
    }

    [Fact]
    public void VisitingAddsPagesAndUpdatesCurrent()
    {
        string output = Run("1", "https://youtube.com", "Video", "1", "https://github.com", "Code", "5");
        Assert.Contains("Visited: https://github.com", output);
        Assert.Contains("[CURRENT] #3  https://github.com", output);
        Assert.Contains("#2  https://youtube.com", output);
    }

    [Theory]
    [InlineData("100", "https://example.com")]
    [InlineData("1", "https://youtube.com")]
    public void GoingBackStopsAtExpectedPage(string steps, string expected)
    {
        string output = Run("1", "https://youtube.com", "", "1", "https://github.com", "", "2", steps);
        Assert.Contains($"Now at: {expected}", output);
    }

    [Fact]
    public void ForwardNavigationStopsAtEnd()
    {
        string output = Run("1", "https://youtube.com", "", "2", "1", "3", "100");
        Assert.Contains("Now at: https://youtube.com", output);
    }

    [Fact]
    public void VisitingFromMiddleDiscardsForwardBranch()
    {
        string output = Run("1", "https://youtube.com", "", "1", "https://github.com", "", "2", "1",
                            "1", "https://ada.edu.az", "", "5");
        Assert.Contains("[CURRENT] #4  https://ada.edu.az", output);
        Assert.DoesNotContain("#3  https://github.com", output);
    }

    [Fact]
    public void DeletingUrlRemovesNodeAndIncreasesUndoCount()
    {
        string output = Run("1", "https://youtube.com", "", "6", "https://youtube.com", "11");
        Assert.Contains("URL deleted and pushed to restore stack", output);
        Assert.Contains("Undo stack: 1", output);
    }

    [Fact]
    public void RestoreReturnsDeletedUrl()
    {
        string output = Run("1", "https://youtube.com", "", "6", "https://youtube.com", "7", "5");
        Assert.Contains("Restored latest deleted URL", output);
        Assert.Contains("#2  https://youtube.com", output);
    }

    [Fact]
    public void ClearHistoryRemovesAllPages()
    {
        string output = Run("8", "5", "4");
        Assert.Contains("History and restore stack cleared", output);
        Assert.Contains("(History is empty)", output);
    }

    [Fact]
    public void BadStepCountIsRejected()
    {
        string output = Run("2", "banana");
        Assert.Contains("Error: Enter an integer number of steps", output);
    }

    [Fact]
    public void ImportExportRoundTripPreservesCurrentPage()
    {
        string file = Path.Combine(Path.GetTempPath(), $"bh_xunit_{Guid.NewGuid():N}.txt");
        try
        {
            string output = Run("1", "https://youtube.com", "Video", "9", file,
                                "8", "10", file, "4", "11");
            Assert.Contains("Imported 2 page(s)", output);
            Assert.Contains("Current: #2 | URL: https://youtube.com", output);
            Assert.Contains("Pages: 2/50", output);
        }
        finally { if (File.Exists(file)) File.Delete(file); }
    }

    [Fact]
    public void InvalidImportLeavesOriginalHistoryIntact()
    {
        string output = Run("10", "this-file-does-not-exist-00000.txt", "4");
        Assert.Contains("Error: Cannot open import file", output);
        Assert.Contains("Current: #1 | URL: https://example.com", output);
    }
}
