#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright 2021 David Guillen Fandos <david@davidgf.net>
# Released under the GPL2 license

# This is a small script to regression test ROMs with a libretro core by using
# miniretro as the driving frontend.
# Please see README for instructions!


from multiprocessing.pool import ThreadPool
import argparse, os, subprocess, random, hashlib, time, json
from tqdm import tqdm

_ROM_HASH_PREFIX = 32 * 1024 * 1024

parser = argparse.ArgumentParser(prog='regression.py')
parser.add_argument('--core', dest='core', required=True, help='Core (.so file) to test')
parser.add_argument('--system', dest='system', required=True, help='system directoy')
parser.add_argument('--driver', dest='driver', type=str, default="./miniretro", help='Miniretro driver commandline')
parser.add_argument('--frames', dest='frames', type=int, default=3200, help='Frames per game to run')
parser.add_argument('--capture', dest='capture', type=int, default=1, help='Number of frames to capture')
parser.add_argument('--random-capture', dest='randomcapture', type=int, default=0, help='Number of pseudo-random frames to capture')
parser.add_argument('--record', dest='record', action="store_true", help='Record video and audio')
parser.add_argument('--threads', dest='threads', type=int, default=8, help='CPUs (threads) to use')
parser.add_argument('--input', dest='infiles', nargs='+', help='Set of files or directories to use as test files')
parser.add_argument('--output', dest='output', required=True, help='Output report file (either .txt or .html)')
args = parser.parse_args()

roms = []
for elem in args.infiles:
  if os.path.isfile(elem):
    roms.append(elem)
  else:
    for root, dirs, files in os.walk(os.path.abspath(elem), topdown=False):
      for name in files:
        if name and name[0] == '.': continue
        roms.append(os.path.join(root, name))

os.mkdir(args.output)
# Fake controls :)
ctrl = ["%d:a %d:a %d:a %d:a %d:start %d:start %d:start %d:start" % (
   i, i+1, i+2, i+30, i+60, i+90, i+91, i+92) for i in range(0, args.frames, 300)]
# Take the last frame and N-1 frames from start too
frameevery = args.frames // args.capture if args.capture else args.frames + 100

def rndnums(seed, cnt):
  a = 1140671485
  c = 128201163
  m = 2**24
  r = []
  for _ in range(cnt):
    seed = (a*seed + c) & (m - 1)
    r.append(seed)
  return r

def runcore(rom):
  h = hashlib.sha1(open(rom, "rb").read(_ROM_HASH_PREFIX))
  romid = h.hexdigest()[:12]
  seed = int.from_bytes(h.digest()[:3], byteorder='big', signed=False)
  opath = os.path.join(args.output, romid)
  os.mkdir(opath)
  vfile = os.path.join(opath, "video.mp4")
  afile = os.path.join(opath, "audio.ogg")
  rfile = os.path.join(opath, "video.mkv")
  eargs = []

  if args.record:
    eargs += [
      "--dump-video", vfile,
      "--dump-audio", afile,
    ]
  if args.randomcapture:
    eargs += ["--dump-frames"] + [str(x % args.frames) for x in rndnums(seed, args.randomcapture)]

  with open(os.path.join(opath, "stdout"), "wb") as stdout:
    with open(os.path.join(opath, "stderr"), "wb") as stderr:
      starttime = time.time()
      spcall = subprocess.Popen(
        args.driver.split(" ") + 
        ["--core", args.core,
         "--rom", rom,
         "--output", opath,
         "--system", args.system,
         "--input", " ".join(ctrl),
         "--dump-frames-every", str(frameevery),
         "--frames", str(args.frames + 3),  # Ensure we get a final frame
         ] + eargs,
        stdout=stdout, stderr=stderr,
        preexec_fn=lambda : os.nice(10))
      spcall.wait()
      endtime = time.time()
  subprocess.Popen(["gzip", "-5", os.path.join(opath, "stdout")]).wait()
  subprocess.Popen(["gzip", "-5", os.path.join(opath, "stderr")]).wait()
  with open(os.path.join(opath, "results.json"), "w") as metafd:
    metafd.write(json.dumps({
      "runtime": endtime - starttime,
      "exitcode": spcall.returncode,
      "rom": os.path.basename(rom),
    }))
  if args.record and os.path.isfile(vfile) and os.path.isfile(afile):
    spcall = subprocess.Popen(
      ["ffmpeg",
       "-i", vfile, "-i", afile,
       "-c:v", "copy", "-c:a", "copy",
       rfile],
       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
       preexec_fn=lambda : os.nice(10))
    spcall.wait()
    os.unlink(vfile)
    os.unlink(afile)

  return romid

results = []
tp = ThreadPool(args.threads)
for rom in sorted(roms):
  results.append(tp.apply_async(runcore, (rom,)))

run_results = []
for r in tqdm(results):
  r.wait()
  run_results.append(r.get())
  # Update the results every time so that we can see intermediate results
  with open(os.path.join(args.output, "results.json"), "w") as metafd:
    metafd.write(json.dumps(run_results))

tp.close()
tp.join()


