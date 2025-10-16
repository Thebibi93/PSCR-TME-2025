#include "Tasks.h"
#include "util/ImageUtils.h"
#include "util/thread_timer.h"
#include <csignal>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <qglobal.h>
#include <qimage.h>
#include <qimagereader.h>
#include <sstream>
#include <system_error>
#include <thread>

namespace pr {

void treatImage(FileQueue &fileQueue, const std::filesystem::path &outputFolder) {
  // measure CPU time in this thread
  pr::thread_timer timer;

  while (true) {
    std::filesystem::path file = fileQueue.pop();
    if (file == pr::FILE_POISON)
      break; // poison pill
    QImage original = pr::loadImage(file);
    if (!original.isNull()) {
      QImage resized = pr::resizeImage(original);
      std::filesystem::path outputFile = outputFolder / file.filename();
      pr::saveImage(resized, outputFile);
    }
  }

  // trace
  std::stringstream ss;
  ss << "Thread " << std::this_thread::get_id() << " (treatImage): " << timer << " ms CPU"
     << std::endl;
  std::cout << ss.str();
}

void reader(FileQueue &fileQueue, ImageTaskQueue &imageQueue) {
  pr::thread_timer timer;
  while (true) {
    std::filesystem::path path = fileQueue.pop();
    if (path == FILE_POISON) {
      break;
    }

    QImage image = loadImage(path);
    if (image.isNull()) {
      std::cerr << path << ": no such file" << std::endl;
      continue;
    }
    auto imgPtr = std::make_unique<QImage>(std::move(image));

    TaskData *task = new TaskData(path, std::move(imgPtr));
    imageQueue.push(task);
  }
  imageQueue.push(TASK_POISON);
  std::stringstream ss;
  ss << "Thread " << std::this_thread::get_id() << " (reader): " << timer << " ms CPU" << std::endl;
  std::cout << ss.str();
}

void resizer(ImageTaskQueue &imageQueue, ImageTaskQueue &resizedQueue) {
  pr::thread_timer timer;
  while (true) {
    pr::TaskData *img_task = imageQueue.pop();
    if (img_task == TASK_POISON) {
      break;
    }
    const QImage *ori_img = img_task->image.get();
    QImage resized = pr::resizeImage(*ori_img);

    auto resizedPtr = std::make_unique<QImage>(std::move(resized));
    TaskData *resizedTask = new TaskData(img_task->filePath, std::move(resizedPtr));
    resizedQueue.push(resizedTask);

    delete img_task; // Free img_task alloue avec new
  }
  resizedQueue.push(TASK_POISON);
  std::stringstream ss;
  ss << "Thread " << std::this_thread::get_id() << " (resizer): " << timer << " ms CPU"
     << std::endl;
  std::cout << ss.str();
}

void saver(ImageTaskQueue &resizedQueue, const std::filesystem::path &outputFolder) {
  pr::thread_timer timer;
  while (true) {
    pr::TaskData *img_task = resizedQueue.pop();
    if (img_task == TASK_POISON) {
      break;
    }
    const QImage *res_img = img_task->image.get();
    std::filesystem::path outputFile = outputFolder / img_task->filePath.filename();
    pr::saveImage(*res_img, outputFile);
    delete img_task;
  }
  std::stringstream ss;
  ss << "Thread " << std::this_thread::get_id() << " (saver): " << timer << " ms CPU" << std::endl;
  std::cout << ss.str();
}

} // namespace pr