#pragma once

#include <Scene.h>
#include <Image.h>
namespace pr {

class Job {
public:
	virtual void run () = 0;
	virtual ~Job() {};
};

class PixelJob : public Job {
	int x;
	int y;
	const Scene &scene;
	Image &img;

	void calc_pix(int x, int y, const Scene &scene, Image &img){
		const Scene::screen_t &screen = scene.getScreenPoints();
		auto &screenPoint = screen[y][x];
        // le rayon a inspecter
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
	}
	public :
	void run () {
		calc_pix(x,y, scene, img);
	}
	PixelJob(int x, int y, Scene &scene, Image &img) : x(x), y(y), scene(scene), img(img) {}
	~PixelJob(){}
};

// Job concret : exemple

/**
class SleepJob : public Job {
	int calcul (int v) {
		std::cout << "Computing for arg =" << v << std::endl;
		// traiter un gros calcul
		this_thread::sleep_for(1s);
		int ret = v % 255;
		std::cout << "Obtained for arg =" << arg <<  " result " << ret << std::endl;
		return ret;
	}
	int arg;
	int * ret;
public :
	SleepJob(int arg, int * ret) : arg(arg), ret(ret) {}
	void run () {
		* ret = calcul(arg);
	}
	~SleepJob(){}
};
**/

}
