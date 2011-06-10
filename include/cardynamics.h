#ifndef _CARDYNAMICS_H
#define _CARDYNAMICS_H

#include "rigidbody.h"
#include "carengine.h"
#include "carclutch.h"
#include "cartransmission.h"
#include "cardifferential.h"
#include "carfueltank.h"
#include "carsuspension.h"
#include "carwheel.h"
#include "cartire.h"
#include "carbrake.h"
#include "carwheelposition.h"
#include "caraerodynamicdevice.h"
#include "joeserialize.h"
#include "macros.h"
#include "collision_contact.h"
#include "BulletDynamics/Dynamics/btActionInterface.h"
#include "LinearMath/btDefaultMotionState.h"

#ifdef _MSC_VER
#include <memory>
#else
#include <tr1/memory>
#endif

class MODEL;
class CONFIG;
class COLLISION_WORLD;
class btMultiSphereShape;

class CARDYNAMICS : public btActionInterface
{
friend class PERFORMANCE_TESTING;
friend class joeserialize::Serializer;
public:
	CARDYNAMICS();

	~CARDYNAMICS();
	
	bool Load(
		const CONFIG & cfg,
		const btVector3 & size,
		const btVector3 & center,
		const btVector3 & position,
		const btQuaternion & rotation,
		COLLISION_WORLD & world,
		std::ostream & error_output);
	
	// bullet interface
	void updateAction(btCollisionWorld * collisionWorld, btScalar dt);
	void debugDraw(btIDebugDraw * debugDrawer);

	// graphics interpolated
	void Update();
	const btVector3 & GetPosition() const;
	const btQuaternion & GetOrientation() const;
	btVector3 GetEnginePosition() const;
	btVector3 GetWheelPosition(WHEEL_POSITION wp) const;
	btVector3 GetWheelPosition(WHEEL_POSITION wp, btScalar displacement_percent) const; // for debugging
	btQuaternion GetWheelOrientation(WHEEL_POSITION wp) const;
	btQuaternion GetUprightOrientation(WHEEL_POSITION wp) const;

	// collision world interface
	const COLLISION_CONTACT & GetWheelContact(WHEEL_POSITION wp) const;
	COLLISION_CONTACT & GetWheelContact(WHEEL_POSITION wp);

	// body
	const btVector3 & GetWheelVelocity(WHEEL_POSITION wp) const;
	const btVector3 & GetCenterOfMassPosition() const;
	const btVector3 & GetVelocity() const;
	btScalar GetInvMass() const;
	btScalar GetSpeed() const;
	btScalar GetLateralVelocity() const;

	// driveline control
	void StartEngine();
	void ShiftGear(int value);
	void SetThrottle(btScalar value);
	void SetClutch(btScalar value);
	void SetBrake(btScalar value);
	void SetHandBrake(btScalar value);
	void SetAutoClutch(bool value);
	void SetAutoShift(bool value);

	// first wheel velocity
	btScalar GetSpeedMPS() const;

	// engine rpm
	btScalar GetTachoRPM() const;

	// driveline state access
	const CARENGINE & GetEngine() const {return engine;}
	const CARCLUTCH & GetClutch() const {return clutch;}
	const CARTRANSMISSION & GetTransmission() const {return transmission;}
	const CARBRAKE & GetBrake(WHEEL_POSITION pos) const {return brake[pos];}
	const CARWHEEL & GetWheel(WHEEL_POSITION pos) const {return wheel[pos];}
	const CARTIRE & GetTire(WHEEL_POSITION pos) const {return tire[pos];}

	// traction control
	void SetABS(const bool newabs);
	bool GetABSEnabled() const;
	bool GetABSActive() const;
	void SetTCS(const bool newtcs);
	bool GetTCSEnabled() const;
	bool GetTCSActive() const;

	// set body position
	void SetPosition(const btVector3 & pos);

	// move the car along z-axis until it is touching the ground
	void AlignWithGround();

	// rotate car back onto it's wheels after rollover
	void RolloverRecover();

	// set the steering angle to "value", where 1.0 is maximum right lock and -1.0 is maximum left lock.
	void SetSteering(const btScalar value);

	// get the maximum steering angle in degrees
	btScalar GetMaxSteeringAngle() const;

	const CARSUSPENSION & GetSuspension(WHEEL_POSITION pos) const {return *suspension[pos];}

	btScalar GetAerodynamicDownforceCoefficient() const;

	btScalar GetAeordynamicDragCoefficient() const;

	btVector3 GetTotalAero() const;
	
	btScalar GetFeedback() const;

	//void UpdateTelemetry(btScalar dt);

	// print debug info to the given ostream.  set p1, p2, etc if debug info part 1, and/or part 2, etc is desired
	void DebugPrint(std::ostream & out, bool p1, bool p2, bool p3, bool p4) const;

	bool Serialize(joeserialize::Serializer & s);

protected:
	// body state
	COLLISION_WORLD * world;
	btMultiSphereShape * shape;
	btRigidBody * body;
	btDefaultMotionState motionState;	// common implementation to synchronize world transforms with offsets
	btVector3 center_of_mass;
	btTransform transform;				// last body transform

	// interpolated state
	btVector3 bodyPosition;
	btQuaternion bodyRotation;

	// driveline state
	CARENGINE engine;
	CARFUELTANK fuel_tank;
	CARCLUTCH clutch;
	CARTRANSMISSION transmission;
	CARDIFFERENTIAL differential_front;
	CARDIFFERENTIAL differential_rear;
	CARDIFFERENTIAL differential_center;
	btAlignedObjectArray<CARBRAKE> brake;
	btAlignedObjectArray<CARWHEEL> wheel;
	btAlignedObjectArray<CARTIRE> tire;

	enum { NONE = 0, FWD = 1, RWD = 2, AWD = 3 } drive;
	btScalar driveshaft_rpm;
	btScalar tacho_rpm;

	bool autoclutch;
	bool autoshift;
	bool shifted;
	int shift_gear;
	btScalar last_auto_clutch;
	btScalar remaining_shift_time;

	// traction control state
	bool abs;
	bool tcs;
	std::vector <int> abs_active;
	std::vector <int> tcs_active;

	// suspension
	btAlignedObjectArray<btVector3> wheel_velocity;
	btAlignedObjectArray<btVector3> wheel_position;
	btAlignedObjectArray<btQuaternion> wheel_orientation;
	btAlignedObjectArray<COLLISION_CONTACT> wheel_contact;
	std::vector<std::tr1::shared_ptr<CARSUSPENSION> > suspension;

	btAlignedObjectArray<CARAERO> aerodynamics;
	btAlignedObjectArray<std::pair<btScalar, btVector3> > mass_particles;
	//std::list<CARTELEMETRY> telemetry;
	
	btScalar maxangle;
	btScalar feedback;

	btVector3 GetDownVector() const;

	btVector3 LocalToWorld(const btVector3 & local) const;

	btQuaternion LocalToWorld(const btQuaternion & local) const;

	void UpdateWheelVelocity();

	void UpdateWheelTransform();

	// apply engine torque to body
	void ApplyEngineTorqueToBody();

	// add aerodynamic force / torque to force, torque
	void AddAerodynamics(btVector3 & force, btVector3 & torque);

	// update suspension, sets normal force
	void UpdateSuspension(btScalar normal_force[], btScalar dt);

	// apply tire friction to body
	void UpdateWheel(
		const int i,
		const btScalar dt,
		const btScalar normal_force,
		const btScalar drive_torque,
		const btQuaternion & wheel_space);

	// advance body(body, suspension, wheels) simulation by dt
	void UpdateBody(
		const btVector3 & ext_force,
		const btVector3 & ext_torque,
		const btScalar drive_torque[],
		const btScalar dt);

	// cardynamics
	void Tick(const btScalar dt);

	void UpdateWheelContacts();

	void InterpolateWheelContacts();

	void CalculateMass(
		btVector3 & center,
		btVector3 & inertia,
		btScalar & mass);

	// update engine, return wheel drive torque
	void UpdateDriveline(btScalar drive_torque[], btScalar dt);

	// calculate wheel drive torque
	void CalculateDriveTorque(btScalar drive_torque[], btScalar clutch_torque);

	// calculate driveshaft speed given wheel angular velocity
	btScalar CalculateDriveshaftSpeed();

	// calculate throttle, clutch, gear
	void UpdateTransmission(btScalar dt);

	bool WheelDriven(int i) const;

	btScalar AutoClutch(btScalar last_clutch, btScalar dt) const;

	btScalar ShiftAutoClutch() const;

	btScalar ShiftAutoClutchThrottle(btScalar throttle, btScalar dt);

	// calculate next gear based on engine rpm
	int NextGear() const;

	// calculate downshift point based on gear, engine rpm
	btScalar DownshiftRPM(int gear) const;

	// do traction control system calculations and modify the throttle position if necessary
	void DoTCS(int i);

	// do anti-lock brake system calculations and modify the brake force if necessary
	void DoABS(int i);

	// cardynamics initialization
	void GetCollisionBox(
		const btVector3 & bodySize,
		const btVector3 & bodyCenter,
		btVector3 & center,
		btVector3 & size);

	void Init(
		COLLISION_WORLD & world,
		const btVector3 & bodySize,
		const btVector3 & bodyCenter,
		const btVector3 & position,
		const btQuaternion & rotation);

	void AddMassParticle(const btScalar mass, const btVector3 & pos);
};

#endif
