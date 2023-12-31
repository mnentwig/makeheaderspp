# makeheaderspp "altclass" feature
Simulates Java/.NET interfaces for selective access to objects
### Purpose
Restricting access to manage complexity is one of the main drivers for an object-oriented language like C++. Unfortunately, access control via "public", "protected", "private" is not particularly fine-grained. Other languages like Java or C# allow passing objects via an "interface" which allows access only to a subset of the object's features.

The "altclass" mechanism simulates this feature. Each "interface" to a main class is implemented as a baseclass, with accessible methods declared as virtual. They are overloaded by the main class. 
To provide a limited interface, the mainclass object is upcast to the desired interface base class. Calling a method still invokes the main class implementation, as methods are virtual.

The example illustrates a class with two limited "interfaces" having one commonly available function getObjectHashKey(). Note the `"altclass=xyz"` tags.
```
#include <memory>
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
```
### Functionality
The `altclass` tag simply copies the virtual method declaration to one or more additional classes.