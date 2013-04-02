#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static Metadata M = Metadata()
	.title("noon")
	.package("com.sifteo.noon", "0.1")
	.icon(Icon)
	.cubeRange(1, 1);


void main() {
	static VideoBuffer vid;
	static MotionBuffer<256> motion;
	static MotionIterator iter(motion);

	CubeID cube = 0;

    vid.initMode(BG0_ROM);
    vid.attach(cube);
    vid.bg0rom.erase(BG0ROMDrawable::SOLID_FG ^ BG0ROMDrawable::BLACK);
    vid.bg0rom.fill(vec(0,0), vec(3,3),
                    BG0ROMDrawable::SOLID_FG ^ BG0ROMDrawable::RED);
    motion.attach(cube, 500);

	for(;;) {
        static float oldZ = 0;
        const float COEF = 0.6;

		while(iter.next()) {
            float newZ = iter.accel().z * COEF + oldZ * (1.0-COEF); // smooth
			LOG("%d,%d\n", cube.isTouching(), int(newZ));
            oldZ = newZ;
        }

        auto accelf = cube.accel().toFloat() / 2.f;
        const Int2 center = LCD_center - vec(24,24)/2;
        vid.bg0rom.setPanning(-(center + accelf.xy()));

		System::paint();
	}
}
