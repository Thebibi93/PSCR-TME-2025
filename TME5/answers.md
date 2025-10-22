# TME5 Correction: Parallelization of a Ray Tracer

## Baseline Sequential

### Question 1
Extracted TME5.zip from Moodle, added to repo, committed and pushed.

Configured project with CMake as previous TMEs. No dependencies, all handmade.

### Question 2

Ran `build/TME5` and generated `spheres.bmp`.

Platform: Release, Ubuntu (Linux)

Temps/baseline choisi : 
Ray tracer starting with output 'spheres.bmp', resolution 2000x2000, spheres 250, mode sequential
Total time 2423ms.
# System Details Report
---

## Report details
- **Date generated:**                              2025-10-22 11:13:06

## Hardware Information:
- **Hardware Model:**                              Lenovo IdeaPad Flex 5 14ALC05
- **Memory:**                                      16,0 GiB
- **Processor:**                                   AMD Ryzen™ 7 5700U with Radeon™ Graphics × 16
- **Graphics:**                                    AMD Radeon™ Graphics
- **Disk Capacity:**                               512,1 GB

## Software Information:
- **Firmware Version:**                            GJCN27WW
- **OS Name:**                                     Ubuntu 24.04.3 LTS
- **OS Build:**                                    (null)
- **OS Type:**                                     64-bit
- **GNOME Version:**                               46
- **Windowing System:**                            Wayland
- **Kernel Version:**                              Linux 6.8.0-85-generic

## With Manual Threads

### Question 3
Implemented `void renderThreadPerPixel(const Scene& scene, Image& img)` in Renderer.

mesures
Ray tracer starting with output 'spheres.bmp', resolution 500x500, spheres 250, mode ThreadPerPixel
Total time 13355ms.
Crash a 1000 car c'est quadratique le nb de threads...


### Question 4
Implemented `void renderThreadPerRow(const Scene& scene, Image& img)` in Renderer.

mesures
Ray tracer starting with output 'spheres.bmp', resolution 500x500, spheres 250, mode ThreadPerRow
Total time 91ms.

Ray tracer starting with output 'spheres.bmp', resolution 2000x2000, spheres 250, mode ThreadPerRow
Total time 894ms.

### Question 5
Implemented `void renderThreadManual(const Scene& scene, Image& img, int nbthread)` in Renderer.

mesures


Ray tracer starting with output 'spheres.bmp', resolution 2000x2000, spheres 250, mode ThreadManual, threads 4
Total time 1500ms.

Ray tracer starting with output 'spheres.bmp', resolution 2000x2000, spheres 250, mode ThreadManual, threads 32
Total time 810ms.


## With Thread Pool

### Question 6
Queue class: blocking by default, can switch to non-blocking.

### Question 7
Pool class: constructor with queue size, start, stop.
Job abstract class with virtual run().

### Question 8
PixelJob: derives from Job, captures ?TODO?

renderPoolPixel: 

Mode "-m PoolPixel" with -n.

mesures

### Question 9
LineJob: derives from Job, captures TODO

renderPoolRow: 

Mode "-m PoolRow -n nbthread".

mesures

### Question 10
Best:

## Bonus

### Question 11

pool supportant soumission de lambda.