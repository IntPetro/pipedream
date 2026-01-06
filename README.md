# Pipedream

A multi-threaded, split-based CLI downloader built out of pure frustration with Chrome’s inbuilt downloader.

Written entirely in C++, with one goal:  
**push the network and disk as close to saturation as possible**.

## Why tho

Chrome is safe, polite, and slow.  
IDM is fast, but opaque.

I wanted to understand **why** download managers are fast at the TCP and OS and then build one myself from first principles.

This project is the result.

--

## Core ideas

- Few **persistent TCP connections** instead of many short-lived ones  
- Logical **chunks for scheduling**, not fake parallelism  
- Minimal I/O overhead (no worker logging, no UI work)
- Preallocated output files for parallel random-access writes

---

## Features

- Multi-threaded HTTP/HTTPS downloads using range requests
- Persistent connections per worker
- Chunk scheduling via a shared job queue
- Live throughput monitoring with near-zero overhead
- Preallocated output files for parallel writes
- Resume support (single-threaded for now, will add multi-threaded resume support soon)
- Graceful fallback if range requests aren’t supported

---

## Performance (real-world tests)

All tests use a **1 GB file**, multiple runs. (median shown here)

| Host (Location) | Pipedream | IDM | Chrome |
|-----------------|-----------|-----|--------|
| Vodafone (UK) | 45.1s | 40.3s | 88s |
| Hetzner (Singapore) | 46.2s | 43.2s | 52s |
| Virtua (Frankfurt) | 39.4s | 35.2s | 131s |
| Datapacket (Stockholm) | 66.0s | 48.0s | 70s |

---

## What this is NOT

- Not a Chrome replacement
- Not a full IDM clone
- Not tuned for unstable or lossy networks (yet)

---

## Build

Windows (MSVC):

```bash
cl /std:c++20 /O2 *.cpp winhttp.lib

---

## Future Additions

- Multithreaded resume support
- Focus on correctness
- Handling redirects, unstable servers, different file names reported etc
- Dynamic chunking
