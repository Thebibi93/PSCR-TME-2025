#pragma once

#include "Image.h"
#include "Job.h"
#include "Pool.h"
#include "Ray.h"
#include "Scene.h"
#include <thread>
#include <vector>

namespace pr {

// Classe pour rendre une scène dans une image
class Renderer {
public:
  // Rend la scène dans l'image
  void render(const Scene &scene, Image &img) {
    // les points de l'ecran, en coordonnées 3D, au sein de la Scene.
    // on tire un rayon de l'observateur vers chacun de ces points
    const Scene::screen_t &screen = scene.getScreenPoints();

    // pour chaque pixel, calculer sa couleur
    for (int x = 0; x < scene.getWidth(); x++) {
      for (int y = 0; y < scene.getHeight(); y++) {
        // le point de l'ecran par lequel passe ce rayon
        auto &screenPoint = screen[y][x];
        // le rayon a inspecter
        Ray ray(scene.getCameraPos(), screenPoint);

        int targetSphere = scene.findClosestInter(ray);

        if (targetSphere == -1) {
          // keep background color
          continue;
        } else {
          const Sphere &obj = scene.getObject(targetSphere);
          // pixel prend la couleur de l'objet
          Color finalcolor = scene.computeColor(obj, ray);
          // mettre a jour la couleur du pixel dans l'image finale.
          img.pixel(x, y) = finalcolor;
        }
      }
    }
  }

  void renderThreadPerPixel(const Scene &scene, Image &img) {
    const Scene::screen_t &screen = scene.getScreenPoints();
    std::vector<std::thread> thread_vector;
    for (int x = 0; x < scene.getWidth(); x++) {
      for (int y = 0; y < scene.getHeight(); y++) {
        thread_vector.emplace_back([&screen, &scene, &img, x, y]() {
          auto &screenPoint = screen[y][x];
          Ray ray(scene.getCameraPos(), screenPoint);
          int targetSphere = scene.findClosestInter(ray);
          if (targetSphere == -1) {
            // keep background color
            return;
          } else {
            const Sphere &obj = scene.getObject(targetSphere);
            // pixel prend la couleur de l'objet
            Color finalcolor = scene.computeColor(obj, ray);
            // mettre a jour la couleur du pixel dans l'image finale.
            img.pixel(x, y) = finalcolor;
          }
        });
      }
    }
    for (auto &t : thread_vector) {
      t.join();
    }
  }

  void renderThreadPerRow(const Scene &scene, Image &img) {
    const Scene::screen_t &screen = scene.getScreenPoints();
    std::vector<std::thread> thread_vector;
    for (int x = 0; x < scene.getWidth(); x++) {
      thread_vector.emplace_back([&screen, &scene, &img, x]() {
        for (int y = 0; y < scene.getHeight(); y++) {
          auto &screenPoint = screen[y][x];
          Ray ray(scene.getCameraPos(), screenPoint);
          int targetSphere = scene.findClosestInter(ray);
          if (targetSphere == -1) {
            // keep background color
            continue;
          } else {
            const Sphere &obj = scene.getObject(targetSphere);
            // pixel prend la couleur de l'objet
            Color finalcolor = scene.computeColor(obj, ray);
            // mettre a jour la couleur du pixel dans l'image finale.
            img.pixel(x, y) = finalcolor;
          }
        }
      });
    }
    for (auto &t : thread_vector) {
      t.join();
    }
  }

  void renderThreadManual(const Scene &scene, Image &img, int nbthread) {
    const Scene::screen_t &screen = scene.getScreenPoints();
    int w = scene.getWidth();
    int h = scene.getHeight();
    int lines_per_thread = h / nbthread;

    std::vector<std::thread> thread_vector;
    thread_vector.reserve(nbthread);

    for (int i = 0; i < nbthread; ++i) {
        thread_vector.emplace_back([i, &scene, &screen, &img, nbthread, w, h, lines_per_thread]() {
            if (i == nbthread - 1) {
                for (int y = lines_per_thread * i; y < h; ++y) {
                    auto &row = screen[y];
                    for (int x = 0; x < w; ++x) {
                        const auto &screenPoint = row[x];
                        Ray ray(scene.getCameraPos(), screenPoint);

                        int targetSphere = scene.findClosestInter(ray);
                        if (targetSphere == -1) {
                            continue;
                        } else {
                            const Sphere &obj = scene.getObject(targetSphere);
                            Color finalcolor = scene.computeColor(obj, ray);
                            img.pixel(x, y) = finalcolor;
                        }
                    }
                }
                return;
            }

            for (int y = lines_per_thread * i; y < lines_per_thread * (i + 1); ++y) {
                auto &row = screen[y];
                for (int x = 0; x < w; ++x) {
                    const auto &screenPoint = row[x];
                    Ray ray(scene.getCameraPos(), screenPoint);

                    int targetSphere = scene.findClosestInter(ray);
                    if (targetSphere == -1) {
                        continue;
                    } else {
                        const Sphere &obj = scene.getObject(targetSphere);
                        Color finalcolor = scene.computeColor(obj, ray);
                        img.pixel(x, y) = finalcolor;
                    }
                }
            }
        });
    }

    for (auto &t : thread_vector) {
        t.join();
    }
}

  void renderPoolPixel(const Scene &scene, Image &img, int nbthread) { 
    // Création du pool de thread
    Pool p(scene.getWidth() * scene.getHeight());
    p.start(nbthread);
    for (int x = 0; x<scene.getWidth(); ++x){
      for (int y=0; y<scene.getHeight(); ++y){
        PixelJob *job = new PixelJob(x,y,scene,img);
        p.submit(job);
      }
    }
    p.stop();
  }

  void renderPoolRow(const Scene &scene, Image &img, int nbthread) {
    Pool p (scene.getHeight());
    p.start(nbthread);
    for (int y = 0; y <scene.getHeight(); ++y){
      LineJob *job = new LineJob(y,scene,img);
      p.submit(job);
    }
    p.stop();
  }
};

} // namespace pr