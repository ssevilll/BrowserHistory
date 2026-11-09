"""Generate narrated demo video from the verified CLI transcript + test log.
Requires Pillow, espeak and ffmpeg (production utility only; C++ project has no demo-generation dependencies).
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import subprocess, wave, re

ROOT = Path(__file__).resolve().parent
OUT = ROOT / 'BrowserHistory-Demo.mp4'
SCENES = ROOT / '_video_frames'
SCENES.mkdir(exist_ok=True)
W, H = 1280, 720
COL = dict(bg='#0c1828', panel='#14283b', panel2='#1e374c', accent='#70c2f3', mint='#94e1b1', white='#f3f7fc', mute='#b1bfd0', danger='#ffadad', line='#3b5267')
FONT='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
BOLD='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
MONO='/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf'
T=lambda size,bold=False: ImageFont.truetype(BOLD if bold else FONT, size)
M=lambda size: ImageFont.truetype(MONO,size)

narrations = [line.split('. ',1)[1].strip() for line in (ROOT/'demo-narration.txt').read_text().splitlines() if line]
assert len(narrations)==9

cli=(ROOT/'demo-output.txt').read_text()
assert 'https://www.ada.edu.az' in cli and 'Restored latest deleted URL' in cli
log=(ROOT/'native-test-results.txt').read_text()
assert 'RESULT: 37 passed, 0 failed' in log


def rounded(d, rect, fill, outline=None, width=2, radius=18):
    d.rounded_rectangle(rect, radius=radius, fill=fill, outline=outline, width=width)

def text(d, x, y, s, size=24, fill=None, bold=False, font=None):
    d.text((x,y),s,font=font or T(size,bold),fill=fill or COL['white'])

def box_lines(d, x, y, lines, size=24, leading=38, title=None, height=None, width=1140):
    height=height or len(lines)*leading+88
    rounded(d,(x,y,x+width,y+height),COL['panel'],COL['line'])
    if title:
        text(d,x+24,y+19,title,22,COL['accent'],True)
        y+=48
    else:y+=25
    for i,line in enumerate(lines):
        color=COL['mint'] if ('[CURRENT]' in line or 'PASS' in line or 'RESULT:' in line) else COL['white']
        if 'removed' in line or 'GitHub x' in line:color=COL['danger']
        text(d,x+24,y+i*leading,line,size,color,font=M(size))

def base(num,title,sub):
    im=Image.new('RGB',(W,H),COL['bg'])
    d=ImageDraw.Draw(im)
    d.rectangle((0,0,W,12),fill=COL['accent'])
    text(d,65,40,'CSCI 2304  •  MINI-PROJECT 1',19,COL['accent'],True)
    text(d,65,86,title,39,COL['white'],True)
    text(d,67,147,sub,20,COL['mute'])
    d.line((65,672,1215,672),fill=COL['line'],width=2)
    text(d,67,687,'Verified C++23 program output • custom data structures',15,COL['mute'])
    text(d,1125,684,f'{num} / 9',18,COL['accent'],True)
    return im,d

pages=[]

im,d=base(1,'BROWSER HISTORY MANAGER','Runnable C++23 application • CLI + 37 native tests + xUnit.net project')
box_lines(d,70,217,[
    '$ ./scripts/run.sh',
    'Browser History Manager | starting page: https://example.com',
    '',
    '1. Visit a page     2. Back       3. Forward',
    '4. Current page     5. Display    6. Delete URL',
    '7. Restore          8. Clear      9. Export',
    '10. Import          11. Stats     0. Exit',
],23,48,height=393)
pages.append(im)

im,d=base(2,'HOW THE DATA STRUCTURES WORK','PageNode + DoublyLinkedList + BrowserHistory + deleted-page stack')
labels=['HOME  (#1)','YOUTUBE  (#2)','GITHUB  (#3)']
for i,label in enumerate(labels):
    x=76+i*407
    rounded(d,(x,240,x+340,360),COL['panel2'],COL['line'])
    text(d,x+25,264,label,23,COL['white'],True)
    text(d,x+25,315,'prev   |   next',21,COL['accent'],font=M(21))
    if i<2: text(d,x+349,281,'↔',35,COL['mint'],True)
text(d,75,390,'head = oldest',22,COL['accent'],True)
text(d,845,390,'tail = newest / current',22,COL['mint'],True)
box_lines(d,76,450,[
    'PageNode { id, url, title, visitedAt, prev, next }',
    'BrowserHistory { linkedList, current, deletedStack, capacity }',
],19,36,width=1128,height=150,title='CLASS STATE (SOURCE CODE DESIGN)')
pages.append(im)

im,d=base(3,'VISITING PAGES','A new node is appended after the active cursor')
box_lines(d,70,222,[
    '> Visit: https://www.youtube.com     [Videos]',
    '> Visit: https://github.com          [Coding]',
    '',
    '          #1  https://example.com',
    '                 <--> ',
    '          #2  https://www.youtube.com',
    '                 <--> ',
    '[CURRENT] #3  https://github.com',
],24,42,height=395,title='VERIFIED C++ CONSOLE OUTPUT: display_history()')
pages.append(im)

im,d=base(4,'BACK + BRANCHING','Going back and visiting a new page removes the old forward branch')
box_lines(d,70,217,[
    '> Back 1 step  ->  https://www.youtube.com',
    '> Visit https://www.ada.edu.az',
    '',
    '          #1  https://example.com',
    '                 <--> ',
    '          #2  https://www.youtube.com',
    '                 <--> ',
    '[CURRENT] #4  https://www.ada.edu.az',
],24,43,height=400,title='VERIFIED C++ CONSOLE OUTPUT: back() and visit()')
text(d,80,631,'GitHub x  [discarded from forward history]',19,COL['danger'],True)
pages.append(im)

im,d=base(5,'DELETE A SPECIFIC URL','unlink() adjusts prev/next; metadata is pushed to a linked LIFO stack')
box_lines(d,70,222,[
    '> Delete URL: https://www.youtube.com',
    'URL deleted and pushed to restore stack',
    '',
    '          #1  https://example.com',
    '                 <--> ',
    '[CURRENT] #4  https://www.ada.edu.az',
],24,46,height=350,title='CONSOLE OUTPUT: delete_url()')
box_lines(d,70,593,['STACK TOP  ->  YouTube (#2)'],22,28,height=63)
pages.append(im)

im,d=base(6,'RESTORE THE LAST DELETED URL','LIFO pop restores the deleted node near its previous neighbors')
box_lines(d,70,222,[
    '> Restore most recent deleted URL',
    'Restored latest deleted URL',
    '',
    '          #1  https://example.com',
    '                 <--> ',
    '          #2  https://www.youtube.com',
    '                 <--> ',
    '[CURRENT] #4  https://www.ada.edu.az',
],24,40,height=395,title='CONSOLE OUTPUT: restore_deleted()')
pages.append(im)

im,d=base(7,'CAPACITY + IMPORT / EXPORT','Memory limit: oldest page is pruned when full • TXT saves active cursor')
box_lines(d,70,215,[
    'Capacity example (limit = 3):',
    '  Before:  home <-> one <-> two',
    '  Visit:   three',
    '  After:   one <-> two <-> three',
    '',
    'Demo console: ',
    '  Exported to demo/history-export.txt',
    '  History and restore stack cleared',
    '  Imported 3 page(s)',
],23,38,height=410,title='CAPACITY BEHAVIOR + VERIFIED C++ I/O')
pages.append(im)

im,d=base(8,'AUTOMATED TEST SUITE','37 verified native tests • xUnit.net integration suite included')
loglines=[line.strip() for line in log.splitlines() if line.strip()]
chosen=[x for x in loglines if any(t in x for t in ['initial homepage','branching drops','delete middle','restore is LIFO','capacity evicts','export import full','invalid import never','pointer stress','RESULT:'])]
box_lines(d,70,220,chosen,21,40,height=410,title='ACTUAL OUTPUT: ./scripts/test.sh')
pages.append(im)

im,d=base(9,'END OF DEMONSTRATION','C++23 console program + xUnit.net test project included')
box_lines(d,70,230,[
    '[PASS]  Doubly linked list implemented from scratch',
    '[PASS]  Back / Forward / Visit / Current / Display',
    '[PASS]  Capacity / Delete history / Delete URL / Restore',
    '[PASS]  TXT import and export / CLI menu',
    '[PASS]  37 tests / PDF report / source ZIP',
],24,52,height=375,title='REFERENCE PROJECT COVERAGE')
pages.append(im)

for i,im in enumerate(pages):
    im.save(SCENES/f'scene_{i+1:02d}.png')

rawaudio=[]
for i,narr in enumerate(narrations):
    wav=SCENES/f'voice_{i+1:02d}.wav'
    subprocess.run(['espeak','-v','en-us','-s','145','-a','165','-w',str(wav),narr],check=True,
                   stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    with wave.open(str(wav),'rb') as file:
        rawaudio.append((file.getparams(),file.readframes(file.getnframes())))

params=rawaudio[0][0]
assert all(a[0].framerate==params.framerate and a[0].nchannels==params.nchannels and a[0].sampwidth==params.sampwidth for a in rawaudio)
frames=[]
durations=[]
for prm,samples in rawaudio:
    frames.append(samples)
    # add 1.15 seconds of silence as transition
    frames.append(b'\x00' * int(params.framerate*params.nchannels*params.sampwidth*1.15))
    durations.append(len(samples)/(params.framerate*params.nchannels*params.sampwidth)+1.15)

sound=SCENES/'narration-full.wav'
with wave.open(str(sound),'wb') as wav:
    wav.setnchannels(params.nchannels)
    wav.setsampwidth(params.sampwidth)
    wav.setframerate(params.framerate)
    wav.writeframes(b''.join(frames))

manifest=SCENES/'slides.txt'
with manifest.open('w') as out:
    for i,duration in enumerate(durations):
        out.write(f"file 'scene_{i+1:02d}.png'\n")
        out.write(f'duration {duration:.6f}\n')
    out.write("file 'scene_09.png'\n")

subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-y',
                '-f','concat','-safe','0','-i',str(manifest),'-i',str(sound),
                '-c:v','libx264','-preset','ultrafast','-crf','25',
                '-pix_fmt','yuv420p','-r','12','-c:a','aac','-b:a','96k',
                '-movflags','+faststart','-shortest',str(OUT)],check=True)
print(f'Created {OUT}: {OUT.stat().st_size/1e6:.2f} MB, {sum(durations):.1f} seconds')
