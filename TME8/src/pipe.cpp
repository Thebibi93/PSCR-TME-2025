#include "pipe.h"

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <limits.h> // For PIPE_BUF
#include <linux/limits.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace pr {

// Place this in shared memory !
struct PipeShm {
  char buffer[PIPE_BUF];
  size_t head;      // write position
  size_t tail;      // read position
  size_t count;     // number of bytes in the pipe
  size_t nbReaders; // number of readers
  size_t nbWriters; // number of writers
  sem_t read;
  sem_t write;
  sem_t mutex;
};

// This is per-process handle, not in shared memory
struct Pipe {
  PipeShm *shm; // pointer to shared memory
  int oflags;   // O_RDONLY or O_WRONLY
};

int pipe_create(const char *name) {
  // Construct shared memory name
  char shm_name[256];
  // add a '/' at the beginning for shm_open
  snprintf(shm_name, sizeof(shm_name), "/%s", name);
  // Try to create shared memory with O_CREAT|O_EXCL
  shm_unlink(shm_name);
  int fd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    return -1;
  }
  // Set size of shared memory
  if (ftruncate(fd, sizeof(PipeShm)) < 0) {
    close(fd);
    return -1;
  }
  // Map the shared memory
  void *part = mmap(NULL, sizeof(PipeShm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (part == MAP_FAILED) {
    close(fd);
    return -1;
  }
  // Initialize the PipeShm structure
  PipeShm *shm = new (part) PipeShm();
  shm->head = 0;
  shm->tail = 0;
  shm->count = 0;
  shm->nbReaders = 0;
  shm->nbWriters = 0;

  // Including semaphores
  if (sem_init(&shm->read, 1, 0) < 0) {
    munmap(part, sizeof(PipeShm));
    close(fd);
    return -1;
  }
  if (sem_init(&shm->write, 1, PIPE_BUF) < 0) {
    sem_destroy(&shm->read);
    munmap(part, sizeof(PipeShm));
    close(fd);
    return -1;
  }
  if (sem_init(&shm->mutex, 1, 1) < 0) {
    sem_destroy(&shm->read);
    sem_destroy(&shm->write);
    munmap(part, sizeof(PipeShm));
    close(fd);
    return -1;
  }
  // Unmap and close (setup persists in shared memory)
  munmap(part, sizeof(PipeShm));
  close(fd);

  return 0;
}

Pipe *pipe_open(const char *name, int oflags) {
  // Construct shared memory name
  char shm_name[256];
  snprintf(shm_name, sizeof(shm_name), "/%s", name);

  // Open shared memory (without O_CREAT)
  int fd = shm_open(shm_name, O_RDWR, 0);
  if (fd < 0) {
    errno = ENOENT;
    return nullptr;
  }
  // Map the shared memory
  void *part = mmap(NULL, sizeof(PipeShm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (part == MAP_FAILED) {
    close(fd);
    return nullptr;
  }
  // Can close fd after mmap
  close(fd);
  // Increment nbReaders or nbWriters
  PipeShm *shm = (PipeShm *)part;
  if (sem_wait(&shm->mutex) < 0) {
    munmap(part, sizeof(PipeShm));
    return nullptr;
  }
  if (oflags == O_RDONLY) {
    shm->nbReaders++;
  } else if (oflags == O_WRONLY) {
    shm->nbWriters++;
  } else {
    shm->nbReaders++;
    shm->nbWriters++;
  }
  sem_post(&shm->mutex);
  // Create and return Pipe handle
  Pipe *handle = new Pipe();
  handle->shm = shm;
  handle->oflags = oflags;
  return handle;
}

ssize_t pipe_read(Pipe *handle, void *buf, size_t count) {
  if (handle == nullptr || handle->oflags != O_RDONLY) {
    errno = EBADF;
    return -1;
  } else if (count > PIPE_BUF) {
    errno = EINVAL;
    return -1;
  }

  PipeShm *shm = handle->shm;

  // wait until some data available or no writers
  while (true) {
    if (sem_wait(&shm->read) < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if (sem_wait(&shm->mutex) < 0) {
      sem_post(&shm->read);
      return -1;
    }
    if (shm->count > 0 || shm->nbWriters == 0) {
      break;
    }
    sem_post(&shm->mutex);
  }

  // Check if pipe is empty and no writers : EOF
  if (shm->nbWriters == 0 && shm->count == 0) {
    sem_post(&shm->mutex);
    return 0;
  }

  // Read min(count, shm->count) bytes
  size_t to_read = std::min(count, shm->count);
  char *output = (char *)buf;

  // Handle circular buffer: may need to copy in two parts
  size_t first_chunk = std::min(to_read, (size_t)(PIPE_BUF - shm->tail));
  memcpy(output, &shm->buffer[shm->tail], first_chunk);

  if (first_chunk < to_read) {
    // Wrap around to beginning of buffer
    memcpy(output + first_chunk, &shm->buffer[0], to_read - first_chunk);
  }

  shm->tail = (shm->tail + to_read) % PIPE_BUF;
  shm->count -= to_read;

  // warn other readers/writers if needed
  if (shm->nbReaders > 0) {
    sem_post(&shm->write);
  }
  // reveille le prochain lecteur si il reste qq chose a lire
  if (shm->count > 0) {
    sem_post(&shm->read);
  }

  sem_post(&shm->mutex);

  return to_read;
}

ssize_t pipe_write(Pipe *handle, const void *buf, size_t count) {
  if (handle == nullptr || handle->oflags != O_WRONLY) {
    errno = EBADF;
    return -1;
  } else if (count > PIPE_BUF) {
    errno = EINVAL;
    return -1;
  }

  PipeShm *shm = handle->shm;

  // wait until *enough* space available or no readers
  while (true) {
    if (sem_wait(&shm->write) < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if (sem_wait(&shm->mutex) < 0) {
      sem_post(&shm->write);
      return -1;
    }
    if ((PIPE_BUF - shm->count) >= count || shm->nbReaders == 0) {
      break;
    }
    sem_post(&shm->mutex);
  }

  // Check if no readers => SIGPIPE
  if (shm->nbReaders == 0) {
    sem_post(&shm->mutex);
    kill(0, SIGPIPE);
    errno = EPIPE;
    return -1;
  }

  // Write count bytes
  const char *input = (const char *)buf;

  // Handle circular buffer: may need to copy in two parts
  size_t first_chunk = std::min(count, (size_t)(PIPE_BUF - shm->head));
  memcpy(&shm->buffer[shm->head], input, first_chunk);

  if (first_chunk < count) {
    // Wrap around to beginning of buffer
    memcpy(&shm->buffer[0], input + first_chunk, count - first_chunk);
  }

  shm->head = (shm->head + count) % PIPE_BUF;
  shm->count += count;

  // warn other readers/writers if needed
  if (shm->nbWriters > 0) {
    sem_post(&shm->read);
  }
  // reveille le prochain ecrivain si il reste de la place
  if ((PIPE_BUF - shm->count) >= 1) {
    sem_post(&shm->write);
  }

  sem_post(&shm->mutex);

  return count;
}

int pipe_close(Pipe *handle) {
  if (handle == nullptr) {
    errno = EBADF;
    return -1;
  }

  PipeShm *shm = handle->shm;
  if (sem_wait(&shm->mutex) < 0) {
    return -1;
  }

  // Decrement reader or writer count
  if (handle->oflags == O_RDONLY) {
    if (shm->nbReaders > 0)
      shm->nbReaders--;
  } else {
    if (shm->nbWriters > 0)
      shm->nbWriters--;
  }
  // Warn other process as needed (e.g. if last reader/writer)
  if (shm->nbWriters == 0) {
    sem_post(&shm->read);
  }
  if (shm->nbReaders == 0) {
    sem_post(&shm->write);
  }

  sem_post(&shm->mutex);

  // Unmap memory
  if (munmap((void *)shm, sizeof(PipeShm)) < 0) {
    delete handle;
    return 1;
  }

  // Free handle
  delete handle;

  return 0;
}

int pipe_unlink(const char *name) {
  // Construct shared memory name
  char shm_name[256];
  snprintf(shm_name, sizeof(shm_name), "/%s", name);

  // Open and map the segment to destroy semaphores (Linux)
  int fd = shm_open(shm_name, O_RDWR, 0);
  if (fd >= 0) {
    void *part = mmap(NULL, sizeof(PipeShm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (part != MAP_FAILED) {
      PipeShm *shm = (PipeShm *)part;
      sem_destroy(&shm->read);
      sem_destroy(&shm->write);
      sem_destroy(&shm->mutex);
      munmap(part, sizeof(PipeShm));
    }
    close(fd);
  }

  // Unlink shared memory (this also destroys the embedded semaphores)
  if (shm_unlink(shm_name) < 0) {
    return -1;
  }

  return 0;
}

} // namespace pr