#pragma once

#include "Image.h"
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
    int to_do = w / nbthread;
    std::vector<std::thread> thread_vector;
    for (int i = 0; i < nbthread; ++i) {
      thread_vector.emplace_back([i, &scene, nbthread, &screen, &img, to_do]() {
        if (i == nbthread - 1) {
          for (int x = to_do * i; x < scene.getWidth(); ++x) {
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
          }
          return;
        }
        for (int x = to_do * i; x < to_do * (i + 1); ++x) {
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
        }
      });
    }
    for (auto &t : thread_vector) {
      t.join();
    }
  }
};

} // namespace pr