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
static const unsigned kNoNeighbors = 0xFFFFFFFF;

class SensorListener {
public:
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

        vid[id].initMode(BG0_ROM);
        vid[id].attach(id);
        motion[id].attach(id);

        // Draw the cube's identity
        String<128> str;
        if (id) {
            str << "\n\n\n\n\n";
            str << "     Track\n\n";
            str << "   number: " << cube << "\n";
        } else {
            str << "      0\n\n\n\n\n";
            str << "    CONTROL\n";
            str << "1\n";
            str << "             3\n";
            str << "     CUBE\n\n\n\n\n";
            str << "       2\n";
        }
        vid[cube].bg0rom.text(vec(1,1), str);

        // Draw initial state
        drawNeighbors(cube);

        fx_affected[id] = kNoEffect;
    }

    void onTouch(unsigned id)
    {
        // TODO allow incrementing track count here
    }

    void onAccelChange(unsigned id)
    {
        CubeID cube(id);
        auto accel = cube.accel();

        unsigned changeFlags = motion[id].update();
        if (changeFlags) {
            // was the control cube shaked ?
            if (id == kControlCube && motion[kControlCube].shake) {
                // avoid skip command when modulating effect:
                Neighborhood nb(id);
                if (nb.sys.value == kNoNeighbors) {
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
            // Modulate effect F for group G (format: MGFXXX:YYY:ZZZ)
          LOG("M%d%d%d:%d:%d\r\n", id, fx_affected[id], x, y, z);
        }
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

        if (firstID < CUBE_ALLOCATION)
            drawNeighbors(firstID);
        if (secondID < CUBE_ALLOCATION)
            drawNeighbors(secondID);
    }

    void onNeighborAdd(unsigned firstID, unsigned firstSide, unsigned secondID, unsigned secondSide)
    {
        // Enable effect F for group G (format: EGF)
        if (firstID == kControlCube) {
            LOG("E%d%d\r\n", secondID, firstSide);
            fx_affected[secondID] = firstSide;
        } else if (secondID == kControlCube) {
            LOG("E%d%d\r\n", firstID, secondSide);
            fx_affected[firstID] = secondSide;
        }

        if (firstID < CUBE_ALLOCATION)
            drawNeighbors(firstID);
        if (secondID < CUBE_ALLOCATION)
            drawNeighbors(secondID);
    }

    void drawNeighbors(CubeID cube)
    {
        Neighborhood nb(cube);
        BG0ROMDrawable &draw = vid[cube].bg0rom;

        drawSideIndicator(draw, nb, vec( 1,  0), vec(14,  1), TOP);
        drawSideIndicator(draw, nb, vec( 0,  1), vec( 1, 14), LEFT);
        drawSideIndicator(draw, nb, vec( 1, 15), vec(14,  1), BOTTOM);
        drawSideIndicator(draw, nb, vec(15,  1), vec( 1, 14), RIGHT);
    }

    static void drawSideIndicator(BG0ROMDrawable &draw, Neighborhood &nb,
        Int2 topLeft, Int2 size, Side s)
    {
        unsigned nbColor = draw.BLUE;
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
