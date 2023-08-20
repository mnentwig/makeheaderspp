#include <memory>
// testcase source for MHPP (one file for .h and .cpp content)
#ifndef MHPP
#define MHPP(arg)
#endif

// limited interface for "player" for use by the map subsystem
class myPlayer4map {
    MHPP("begin myPlayer4map") // === autogenerated code. Do not edit ===
    public:
    	virtual void getMapIconColorRGB();
    	virtual void getMapXY();
    	virtual void getObjectHashKey();
    MHPP("end myPlayer4map")
};

// limited interface for "player" for use by the AI subsystem
class myPlayer4AI {
    MHPP("begin myPlayer4AI") // === autogenerated code. Do not edit ===
    public:
    	virtual void getBoundingBoxesForAIVisibilityCheck();
    	virtual void getOrientation();
    	virtual void getObjectHashKey();
    MHPP("end myPlayer4AI")
};

// main class. It is complex and we want to limit access to other parties e.g. "map" and "AI".
// Further, disallowed methods should not clutter IDE command completion
class myPlayer : public myPlayer4map, public myPlayer4AI {
    MHPP("begin myPlayer") // === autogenerated code. Do not edit ===
    public:
    	virtual void getMapIconColorRGB();
    	virtual void getMapXY();
    	virtual void getBoundingBoxesForAIVisibilityCheck();
    	virtual void getOrientation();
    	virtual void getObjectHashKey();
    MHPP("end myPlayer")
};

MHPP("public virtual altclass=myPlayer4map")
void myPlayer::getMapIconColorRGB() {}

MHPP("public virtual altclass=myPlayer4map")
void myPlayer::getMapXY() {}

MHPP("public virtual altclass=myPlayer4AI")
void myPlayer::getBoundingBoxesForAIVisibilityCheck() {}

MHPP("public virtual altclass=myPlayer4AI")
void myPlayer::getOrientation() {}

MHPP("public virtual altclass=myPlayer4AI altclass=myPlayer4map")
void myPlayer::getObjectHashKey() {}

int main(void) {
    myPlayer iPlayer = myPlayer();

    // map gets this
    myPlayer4map* piPlayer4map = static_cast<myPlayer4map*>(&iPlayer);

    // AI gets this
    myPlayer4AI* piPlayer4AI = static_cast<myPlayer4AI*>(&iPlayer);

    // using an IDE, only the "map"-related methods show here...
    piPlayer4map->getMapIconColorRGB();
    piPlayer4map->getMapXY();
    piPlayer4map->getObjectHashKey();

    // ... and only the "AI"-related methods here:
    piPlayer4AI->getBoundingBoxesForAIVisibilityCheck();
    piPlayer4AI->getOrientation();
    piPlayer4AI->getObjectHashKey();
    return 0;
}