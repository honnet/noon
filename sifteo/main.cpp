#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static Metadata M = Metadata()
    .title("noon")
    .package("com.sifteo.noon", "2")
    .icon(Icon)
    .cubeRange(2, 5);

static VideoBuffer vid[CUBE_ALLOCATION];
static TiltShakeRecognizer motion[CUBE_ALLOCATION];

typedef uint8_t EffectID;
static const CubeID kControlCube = 0;
static const EffectID kNoEffect = 0xFF;
static EffectID fx_affected[CUBE_ALLOCATION]; // which effect is affected to which track cube?
static const unsigned kNoNeighbors = 0xFFFFFFFF;
static bool recordMode = false;
static bool isRecording[CUBE_ALLOCATION] = {0};
static bool colorAllowed = false;

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
        if (id != kControlCube) {
            str << "\n\n\n\n\n";
            str << "     Track\n\n";
            str << "   number: " << cube << "\n";
        } else {
            str << "\n\n\n\n\n";
            str << " Mode: ";
            str << (recordMode? "RECORD" : "PLAY  ");
            str << "\n\n\n";
            str << "(touch: toggle\n";
            str << "record / play)\n";
        }
        vid[cube].bg0rom.text(vec(1,1), str);

        // Draw initial state
        drawNeighbors(cube);

        fx_affected[id] = kNoEffect;
    }

    void onTouch(unsigned id)
    {
        CubeID cube(id);

        if (id == kControlCube) {
            if (cube.isTouching()) {
                recordMode = !recordMode;

                String<32> str;
                str << " Mode: ";
                str << (recordMode? "RECORD\n" : "PLAY  \n");
                vid[cube].bg0rom.text(vec(1,6), str);
            }
        } else {
            const unsigned notes[CUBE_ALLOCATION] = {42, 43, 44, 45}; // TODO adapt !
            // mute notes if not recording:
            if (!recordMode || (recordMode && isRecording[id])) {
                if (cube.isTouching()) {
                    // Begin note
                    LOG("B%d\r\n", notes[id-1]);
                } else {
                    // Finish note
                    LOG("F%d\r\n", notes[id-1]);
                }
            }
        }
    }

    int ALWAYS_INLINE constrain(int value, int min, int max)
    {
        if (value <= min) return min;
        if (value >= max) return max;
        return value;
    }

    void onAccelChange(unsigned id)
    {
        CubeID cube(id);
        auto accel = cube.accel();

        unsigned changeFlags = motion[id].update();

        if (id != kControlCube) {
            // Modulate effect for instrument cubes:
            if (!recordMode && fx_affected[id] != kNoEffect) {
                char x = constrain(accel.x + 64, 0, 127); // we get values in the range [-128; 127]
                char y = constrain(accel.y + 64, 0, 127); // but we are only interested by [-64; 64]
                char z = constrain(accel.z + 64, 0, 127); // and want to offet to the range [0; 127]
                // Modulate effect F for group G (format: MGFXXX:YYY:ZZZ)
                LOG("M%d%d%d:%d:%d\r\n", id, fx_affected[id], x, y, z);
            }
        } else {
            // play/record if the control cube is shaked
            if (changeFlags && motion[id].shake) {
                if (cube.isTouching()) {
                    LOG("S\r\n");       // Stop
                } else {
                    if (recordMode) {
                        LOG("R\r\n");   // Record
                    } else {
                        LOG("P\r\n");   // Play
                    }
                }
            }
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
            isRecording[secondID] = false;
        } else if (secondID == kControlCube) {
            LOG("D%d%d\r\n", firstID, secondSide);
            fx_affected[firstID] = kNoEffect;
            isRecording[firstID] = false;
        }

        colorAllowed = false;

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
            isRecording[secondID] = true;
        } else if (secondID == kControlCube) {
            LOG("E%d%d\r\n", firstID, secondSide);
            fx_affected[firstID] = secondSide;
            isRecording[firstID] = true;
        }

        colorAllowed = (firstID == kControlCube || secondID == kControlCube);

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
        unsigned nbColor = recordMode? draw.RED : draw.BLUE;
        bool enabled = nb.hasNeighborAt(s) && colorAllowed;
        draw.fill(topLeft, size,
            nbColor | (enabled ? draw.SOLID_FG : draw.SOLID_BG));
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
