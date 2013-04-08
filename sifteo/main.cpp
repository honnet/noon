#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static Metadata M = Metadata()
    .title("noon")
    .package("com.sifteo.noon", "0.2")
    .icon(Icon)
    .cubeRange(2, CUBE_ALLOCATION);

static VideoBuffer vid[CUBE_ALLOCATION];
static TiltShakeRecognizer motion[CUBE_ALLOCATION];

typedef uint8_t EffectID;
static const CubeID kControlCube = 0;
static const EffectID kNoEffect = 0xFF;
static EffectID fx_affected[CUBE_ALLOCATION]; // which effect is affected to which track cube?
static bool an_fx_is_affected = 0;


class SensorListener {
public:
    struct Counter {
        unsigned touch;
        unsigned neighborAdd;
        unsigned neighborRemove;
    } counters[CUBE_ALLOCATION];

    void install()
    {
        Events::neighborAdd.set(&SensorListener::onNeighborAdd, this);
        Events::neighborRemove.set(&SensorListener::onNeighborRemove, this);
        Events::cubeAccelChange.set(&SensorListener::onAccelChange, this);
        Events::cubeTouch.set(&SensorListener::onTouch, this);
        Events::cubeConnect.set(&SensorListener::onConnect, this);

        // Handle already-connected cubes
        for (CubeID cube : CubeSet::connected()) {
            onConnect(cube);
        }
    }

private:
    void onConnect(unsigned id)
    {
        CubeID cube(id);
        uint64_t hwid = cube.hwID();

        bzero(counters[id]);

        vid[id].initMode(BG0_ROM);
        vid[id].attach(id);
        motion[id].attach(id);

        // Draw the cube's identity
        String<128> str;
        str << "I am cube #" << cube << "\n";
        str << "hwid " << Hex(hwid >> 32) << "\n     " << Hex(hwid) << "\n\n";
        vid[cube].bg0rom.text(vec(1,2), str);

        // Draw initial state for all sensors
        onAccelChange(cube);
        onTouch(cube);
        drawNeighbors(cube);

        fx_affected[id] = kNoEffect;
    }

    void onTouch(unsigned id)
    {
        CubeID cube(id);
        counters[id].touch++;

        String<32> str;
        str << "touch: " << cube.isTouching() <<
            " (" << counters[cube].touch << ")\n";
        vid[cube].bg0rom.text(vec(1,9), str);
    }

    void onAccelChange(unsigned id)
    {
        CubeID cube(id);
        auto accel = cube.accel();

        String<64> str;
        str << "acc: "
            << Fixed(accel.x, 3)
            << Fixed(accel.y, 3)
            << Fixed(accel.z, 3) << "\n";

        unsigned changeFlags = motion[id].update();
        if (changeFlags) {
            // Tilt/shake changed

            auto tilt = motion[id].tilt;
            str << "tilt:"
                << Fixed(tilt.x, 3)
                << Fixed(tilt.y, 3)
                << Fixed(tilt.z, 3) << "\n";

            str << "shake: " << motion[id].shake;

            if (id == kControlCube && motion[kControlCube].shake) {
                if (!an_fx_is_affected) {
                    if(cube.isTouching())
                        LOG("P\r\n"); // Prev scene command
                    else
                        LOG("N\r\n"); // Next scene command
                }
            }
        }

        if (id != kControlCube && fx_affected[id] != kNoEffect) {
            char x = accel.x/2 + 64;
            char y = accel.y/2 + 64;
            char z = accel.z/2 + 64;
            // Effect F modulation for group G (format: EGFXXX:YYY:ZZZ)
            LOG("E%d%d%d:%d:%d\r\n", id, fx_affected[id], x, y, z);
        }

        vid[cube].bg0rom.text(vec(1,10), str);
    }

    void onNeighborRemove(unsigned firstID, unsigned firstSide, unsigned secondID, unsigned secondSide)
    {
        // Disable effect F for group G (format: DGF)
        // the group number is the number of the track cube
        // the effrect number is the number of the side or the controlcube
        if (firstID == kControlCube) {
            LOG("D%d%d\r\n", secondID, firstSide);
            fx_affected[secondID] = kNoEffect;
        } else if (secondID == kControlCube) {
            LOG("D%d%d\r\n", firstID, secondSide);
            fx_affected[firstID] = kNoEffect;
        }
        an_fx_is_affected = 0;

        if (firstID < arraysize(counters)) {
            counters[firstID].neighborRemove++;
            drawNeighbors(firstID);
        }
        if (secondID < arraysize(counters)) {
            counters[secondID].neighborRemove++;
            drawNeighbors(secondID);
        }
    }

    void onNeighborAdd(unsigned firstID, unsigned firstSide, unsigned secondID, unsigned secondSide)
    {
        // Activate effect F for group G (format: AGF)
        // the group number is the number of the track cube
        // the effrect number is the number of the side or the controlcube
        if (firstID == kControlCube) {
            LOG("A%d%d\r\n", secondID, firstSide);
            fx_affected[secondID] = firstSide;
        } else if (secondID == kControlCube) {
            LOG("A%d%d\r\n", firstID, secondSide);
            fx_affected[firstID] = secondSide;
        }
        an_fx_is_affected = 1;

        if (firstID < arraysize(counters)) {
            counters[firstID].neighborAdd++;
            drawNeighbors(firstID);
        }
        if (secondID < arraysize(counters)) {
            counters[secondID].neighborAdd++;
            drawNeighbors(secondID);
        }
    }

    void drawNeighbors(CubeID cube)
    {
        Neighborhood nb(cube);

        String<64> str;
        str << "nb "
            << Hex(nb.neighborAt(TOP), 2) << " "
            << Hex(nb.neighborAt(LEFT), 2) << " "
            << Hex(nb.neighborAt(BOTTOM), 2) << " "
            << Hex(nb.neighborAt(RIGHT), 2) << "\n";

        str << "   +" << counters[cube].neighborAdd
            << ", -" << counters[cube].neighborRemove
            << "\n\n";

        BG0ROMDrawable &draw = vid[cube].bg0rom;
        draw.text(vec(1,6), str);

        drawSideIndicator(draw, nb, vec( 1,  0), vec(14,  1), TOP);
        drawSideIndicator(draw, nb, vec( 0,  1), vec( 1, 14), LEFT);
        drawSideIndicator(draw, nb, vec( 1, 15), vec(14,  1), BOTTOM);
        drawSideIndicator(draw, nb, vec(15,  1), vec( 1, 14), RIGHT);
    }

    static void drawSideIndicator(BG0ROMDrawable &draw, Neighborhood &nb,
        Int2 topLeft, Int2 size, Side s)
    {
        unsigned nbColor = draw.ORANGE;
        draw.fill(topLeft, size,
            nbColor | (nb.hasNeighborAt(s) ? draw.SOLID_FG : draw.SOLID_BG));
    }
};


void main()
{
    static SensorListener sensors;

    sensors.install();

    // We're entirely event-driven. Everything is
    // updated by SensorListener's event callbacks.
    while (1)
        System::paint();
}
