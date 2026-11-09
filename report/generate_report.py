"""Regenerate the PDF report with ReportLab. Edit the placeholders in this file.
This is a reference implementation report; the assignment forbids AI.
"""
from pathlib import Path
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

ROOT=Path(__file__).resolve().parent
GROUP='[GROUP NUMBER]'
MEMBERS=[('[MEMBER 1]','[ID 1]'),('[MEMBER 2]','[ID 2]'),('[MEMBER 3]','[ID 3]')]

pdfmetrics.registerFont(TTFont('DejaVu','/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'))
pdfmetrics.registerFont(TTFont('DejaVu-Bold','/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'))
NAVY=colors.HexColor('#152B42'); BLUE=colors.HexColor('#246F9B')
TEXT=colors.HexColor('#273445'); MUTED=colors.HexColor('#657386')
LINE=colors.HexColor('#D9E4ED'); PALE=colors.HexColor('#EAF3F9')
styles={
 'eyebrow': ParagraphStyle('eye',fontName='DejaVu-Bold',fontSize=9.5,leading=15,textColor=BLUE,spaceAfter=13),
 'title': ParagraphStyle('title',fontName='DejaVu-Bold',fontSize=23,leading=30,textColor=NAVY,spaceAfter=12),
 'subtitle': ParagraphStyle('subtitle',fontName='DejaVu',fontSize=11,leading=16,textColor=MUTED,spaceAfter=16),
 'h': ParagraphStyle('h',fontName='DejaVu-Bold',fontSize=11,leading=17,textColor=BLUE,spaceBefore=13,spaceAfter=7),
 'p': ParagraphStyle('p',fontName='DejaVu',fontSize=8.6,leading=14.5,textColor=TEXT,spaceAfter=8),
 'small': ParagraphStyle('small',fontName='DejaVu',fontSize=7.9,leading=12.8,textColor=TEXT),
 'tiny': ParagraphStyle('tiny',fontName='DejaVu',fontSize=7.4,leading=11.5,textColor=TEXT),
 'th': ParagraphStyle('th',fontName='DejaVu-Bold',fontSize=8,leading=12,textColor=colors.white),
 'note': ParagraphStyle('note',fontName='DejaVu-Bold',fontSize=8.2,leading=13.4,textColor=TEXT),
}
P=lambda s,k='p': Paragraph(s,styles[k])
def tbl(rows, widths, header=True):
 t=Table(rows,colWidths=widths,repeatRows=1 if header else 0,hAlign='LEFT')
 st=[('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),9),('RIGHTPADDING',(0,0),(-1,-1),9),('TOPPADDING',(0,0),(-1,-1),8),('BOTTOMPADDING',(0,0),(-1,-1),8),('LINEBELOW',(0,-1),(-1,-1),.5,LINE)]
 if header: st += [('BACKGROUND',(0,0),(-1,0),NAVY),('ROWBACKGROUNDS',(0,1),(-1,-1),[colors.white,colors.HexColor('#F6FAFD')])]
 t.setStyle(TableStyle(st)); return t

s=[P('CSCI 2304  /  DATA STRUCTURES &amp; ALGORITHMS','eyebrow'),
   P('Browser History Management','title'),
   P('Mini-Project 1  |  C++23 implementation with native and xUnit.net tests','subtitle')]
note=tbl([[P('REFERENCE REPORT - The assignment forbids AI use. Replace names, IDs and group number, and verify all submitted work independently.','note')]], [491],False)
note.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,-1),PALE),('BOX',(0,0),(-1,-1),.5,LINE)]))
s += [note,Spacer(1,12),P(f'<b>Group number:</b> {GROUP}'),
      tbl([[P('Team member','th'),P('Student ID','th')]]+[[P(n,'small'),P(i,'small')] for n,i in MEMBERS],[300,191])]
s += [P('1. Objective','h'),P('Build browser-history navigation with a manually implemented doubly linked list, an active page cursor, forward-branch clearing, maximum capacity, deletion/restoration using a custom stack, and TXT import/export. This is a command-line simulation; it does not load websites.'),
      P('2. Data-structure and class design','h')]
s += [tbl([[P('Class','th'),P('Role','th')],
          [P('PageInfo / PageNode','small'),P('Page ID, URL, title, UTC timestamp; raw prev and next node pointers.','small')],
          [P('DoublyLinkedList','small'),P('Manual allocation, insertion, unlink, head/tail traversal, ownership cleanup and pointer invariants.','small')],
          [P('DeletedHistoryStack','small'),P('Handwritten singly linked LIFO stack storing deleted page and original neighbor IDs.','small')],
          [P('BrowserHistory','small'),P('Active cursor, Visit/Back/Forward, capacity, Delete/Restore and persistence.','small')],
          [P('main.cpp','small'),P('Interactive 12-option command-line menu, including Exit.','small')]], [161,330])]
s += [PageBreak(),P('3. Navigation, branches and boundaries','h'),
      P('<b>Order:</b> the head is the oldest entry and the tail is the newest. Back follows prev; Forward follows next. Both clamp at an endpoint if steps are too large. A zero-step operation has no effect, and negative steps are invalid. The current-page query does not move the cursor.'),
      P('<b>Branching:</b> a new Visit after Back destroys all nodes after the cursor, inserts the new node and makes it active. Local insertion/unlink updates both pointer directions. Forward history is therefore discarded instead of retaining an invalid branch.'),
      P('4. Delete, restore and capacity','h'),
      tbl([[P('Requirement','th'),P('Behavior / documented assumption','th')],
           [P('Capacity','small'),P('Default 50 entries, configurable. The oldest entry at head is pruned on overflow. The handout calls the oldest node tail; this implementation consistently uses head = oldest.','small')],
           [P('Delete specific URL','small'),P('Remove latest matching URL, adjust cursor if needed and push deleted metadata, former neighbor IDs and position into the handmade stack.','small')],
           [P('Restore','small'),P('LIFO pop. Reinsert near surviving original neighbors; otherwise use newer-ID or former-index fallback. If full, evict oldest existing page first.','small')],
           [P('Clear and empty','small'),P('Delete all history and undo entries. No active page remains until a new visit or restore.','small')]], [139,352]),
      P('5. Portable TXT persistence','h'),
      P('A UTF-8 text file stores version header, capacity, current node ID and entries containing ID, quoted URL, quoted title and visit time (UTC epoch seconds). The C++ std::quoted helper preserves spaces and quote characters. Import first creates a temporary list and validates IDs, page count and current cursor; only afterward does it replace the old state. The undo stack is intentionally not persisted.'),
      P('6. Complexity','h'),
      tbl([[P('Action','th'),P('Time','th'),P('Reason','th')],
           [P('Adjacent insertion/removal','small'),P('O(1)','small'),P('Constant pointer edits.','small')],
           [P('Back/Forward s steps','small'),P('O(s)','small'),P('One pointer traversal per step.','small')],
           [P('Visit with k forward nodes','small'),P('O(k)','small'),P('Delete abandoned branch.','small')],
           [P('Delete/Restore','small'),P('O(n)','small'),P('Find URL or neighbor anchors.','small')],
           [P('Display, import/export, clear','small'),P('O(n)','small'),P('Visit all nodes.','small')]], [185,59,247])]
s += [PageBreak(),P('7. Testing and verification','h'),
      P('<b>Native C++ suite:</b> tests/native/BrowserHistoryNativeTests.cpp provides 37 distinct tests, including a deterministic 1,500-operation stress test with invariant checks after each operation. The tests were compiled and executed in the package-creation environment with <b>37 passed, 0 failed</b>. Evidence is in demo/native-test-results.txt.'),
      tbl([[P('Area','th'),P('Examples tested','th')],
           [P('Navigation','small'),P('Homepage, visits, Back/Forward, oversized steps, zero steps and invalid negatives.','small')],
           [P('Branching','small'),P('New visits after Back remove the abandoned forward nodes.','small')],
           [P('Pointer safety','small'),P('Deleting head, tail, middle and active node; latest duplicate URL; structural invariants.','small')],
           [P('Restoration','small'),P('Empty-stack behavior, LIFO order, middle-node reconnection and restore at capacity.','small')],
           [P('Files','small'),P('Round-trip metadata and cursor, empty history, invalid file rollback and undo reset.','small')],
           [P('Stress','small'),P('1,500 seeded mixed navigation, insertions, deletions and restorations.','small')]], [121,370]),
      P('8. xUnit.net test project','h'),
      P('The requested actual <b>xUnit.net</b> tests are provided in <b>tests/xunit/</b>, written in <b>C#</b> with [Fact] and [Theory] attributes. They launch the compiled C++ program as a subprocess, supply menu input and assert on its console output. This respects C++ as the implementation language while using genuine xUnit.net for black-box verification.'),
      P('<b>Verification limitation:</b> the xUnit project is included but could not be run in the creation environment because .NET SDK 8 was not installed. Before use, install .NET SDK 8, build the C++ program and run the xUnit tests. The NuGet packages may need internet access on first restore.'),
      P('9. How to reproduce','h'),
      P('Install CMake 3.20+ and a C++23 compiler. On macOS/Linux run <b>./scripts/run.sh</b> for the console app and <b>./scripts/test.sh</b> for all native tests. With .NET SDK 8 installed, <b>./scripts/test.sh --xunit</b> additionally executes xUnit.net. Windows equivalents are in scripts/*.bat. The sample input, output and narrated demo are in demo/.'),
      P('10. Assumptions and limits','h'),
      P('URLs are accepted as nonblank text without network validation. Delete selects the latest exact-URL match. Import does not restore undo history. Restore does not move the active cursor unless there were no remaining pages. Original node order is approximated if both deletion anchors disappeared. No standard-library linked-list containers are used as core history or undo storage.'),
      Spacer(1,11),P('Evidence: src/*.cpp, include/*.hpp, tests/native/*.cpp, tests/xunit/*.cs, demo/native-test-results.txt','tiny')]

def decorate(canvas,doc):
 canvas.saveState(); w,h=A4
 canvas.setStrokeColor(LINE);canvas.line(52,40,w-52,40)
 canvas.setFont('DejaVu',8);canvas.setFillColor(MUTED)
 canvas.drawString(52,25,'CSCI 2304  |  Browser History Management  |  C++23')
 canvas.drawRightString(w-52,25,f'Page {doc.page}')
 canvas.restoreState()

path=ROOT/'Report.pdf'
SimpleDocTemplate(str(path),pagesize=A4,rightMargin=52,leftMargin=52,topMargin=45,bottomMargin=52,
                  title='Browser History Management - C++23 reference report',author='AI-generated reference').build(s,onFirstPage=decorate,onLaterPages=decorate)
print('Wrote',path,'bytes',path.stat().st_size)
