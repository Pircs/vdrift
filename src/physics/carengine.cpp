/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#include "carengine.h"
#include "cfg/ptree.h"
#include "linearinterp.h"

CarEngineInfo::CarEngineInfo():
	displacement(2E-3),
	maxpower(184000),
	redline(7800),
	rpm_limit(9000),
	idle(0.02),
	start_rpm(1000),
	stall_rpm(350),
	fuel_rate(4E7),
	friction{15.438, 2.387E-3, 7.958E-7},
	inertia(0.25),
	mass(200),
	nos_mass(0),
	nos_boost(0),
	nos_fuel_ratio(5)
{
	// ctor
}

bool CarEngineInfo::Load(const PTree & cfg, std::ostream & error_output)
{
	std::vector<btScalar> pos(3);
	if (!cfg.get("displacement", displacement, error_output)) return false;
	if (!cfg.get("max-power", maxpower, error_output)) return false;
	if (!cfg.get("peak-engine-rpm", redline, error_output)) return false;
	if (!cfg.get("rpm-limit", rpm_limit, error_output)) return false;
	if (!cfg.get("inertia", inertia, error_output)) return false;
	if (!cfg.get("start-rpm", start_rpm, error_output)) return false;
	if (!cfg.get("stall-rpm", stall_rpm, error_output)) return false;
	if (!cfg.get("position", pos, error_output)) return false;
	if (!cfg.get("mass", mass, error_output)) return false;

	// friction (Heywood 1988 tfmep)
	friction[0] = 97000 / (4 * M_PI) * displacement;
	friction[1] = 15.00 / (4 * M_PI) * displacement;
	friction[2] = 0.005 / (4 * M_PI) * displacement;

	// fuel consumption
	btScalar fuel_heating_value = 4.5E7; // Ws/kg
	btScalar engine_efficiency = 0.35;
	cfg.get("fuel-heating-value", fuel_heating_value);
	cfg.get("efficiency", engine_efficiency);
	fuel_rate = 1 /	(engine_efficiency * fuel_heating_value);

	// nos parameters
	cfg.get("nos-mass", nos_mass);
	cfg.get("nos-boost", nos_boost);
	cfg.get("nos-ratio", nos_fuel_ratio);

	// torque
	int curve_num = 0;
	std::vector<btScalar> torque_point(2);
	std::string torque_str("torque-curve-00");
	std::vector<std::pair<btScalar, btScalar> > torque;
	while (cfg.get(torque_str, torque_point))
	{
		torque.push_back(std::pair<btScalar, btScalar>(torque_point[0], torque_point[1]));

		curve_num++;
		std::ostringstream s;
		s << "torque-curve-";
		s.width(2);
		s.fill('0');
		s << curve_num;
		torque_str = s.str();
	}
	if (torque.size() <= 1)
	{
		error_output << "You must define at least 2 torque curve points." << std::endl;
		return false;
	}
	SetTorqueCurve(redline, torque);
	position.setValue(pos[0], pos[1], pos[2]);

	return true;
}

void CarEngineInfo::SetTorqueCurve(
	const btScalar redline,
	const std::vector<std::pair<btScalar, btScalar> > & torque)
{
	torque_curve.Clear();

	assert(torque.size() > 1);

	//ensure we have a smooth curve down to 0 RPM
	if (torque[0].first != 0)
		torque_curve.AddPoint(0,0);

	for (std::vector<std::pair<btScalar, btScalar> >::const_iterator i = torque.begin(); i != torque.end(); ++i)
	{
		torque_curve.AddPoint(i->first, i->second);
	}

	//ensure we have a smooth curve for over-revs
	torque_curve.AddPoint(torque[torque.size() - 1].first + 10000, 0);

	//calculate idle throttle position
	for (idle = 0; idle < 1.0; idle += 0.01)
	{
		if (GetTorque(idle, start_rpm) > -GetFrictionTorque(idle, start_rpm))
		{
			//std::cout << "Found idle throttle: " << idle << ", " << GetTorqueCurve(idle, start_rpm) << ", " << friction_torque << std::endl;
			break;
		}
	}
}

btScalar CarEngineInfo::GetTorque(const btScalar throttle, const btScalar rpm) const
{
	if (rpm < 1) return 0.0;
	return torque_curve.Interpolate(rpm) * throttle;
}

btScalar CarEngineInfo::GetFrictionTorque(btScalar throttle, btScalar rpm) const
{
	btScalar s = rpm < 0 ? -1 : 1;
	rpm = s * rpm;
	btScalar f = friction[0] + friction[1] * rpm + friction[2] * rpm * rpm;
	return -s * f * (1.0 - throttle);
}

CarEngine::CarEngine()
{
	Init(info);
}

void CarEngine::Init(const CarEngineInfo & info)
{
	this->info = info;
	shaft.inv_inertia = 1 / info.inertia;
	combustion_torque = 0;
	friction_torque = 0;
	clutch_torque = 0;

	throttle_position = 0;
	nos_boost_factor = 0;
	nos_mass = info.nos_mass;
	out_of_gas = false;
	rev_limit_exceeded = false;
	stalled = false;
}

btScalar CarEngine::Integrate(btScalar clutch_drag, btScalar clutch_angvel, btScalar dt)
{
	btScalar rpm = GetRPM();

	clutch_torque = clutch_drag;

	btScalar torque_limit = shaft.getMomentum(clutch_angvel) / dt;
	if ((clutch_torque > 0 && clutch_torque > torque_limit) ||
		(clutch_torque < 0 && clutch_torque < torque_limit))
	{
		clutch_torque = torque_limit;
	}

	stalled = (rpm < info.stall_rpm);

	//make sure the throttle is at least idling
	if (throttle_position < info.idle)
		throttle_position = info.idle;

	//engine drive torque
	btScalar rev_limit = info.rpm_limit;
	if (rev_limit_exceeded)
		rev_limit -= 100.0; //tweakable
	rev_limit_exceeded = rpm > rev_limit;

	combustion_torque = info.GetTorque(throttle_position, rpm);

	//nitrous injection
	if (nos_mass > 0 && nos_boost_factor > 0)
	{
		btScalar boost = nos_boost_factor * info.nos_boost;
		combustion_torque += boost / shaft.ang_velocity;

		btScalar fuel_consumed = boost * info.fuel_rate * dt;
		btScalar nos_consumed = info.nos_fuel_ratio * fuel_consumed;
		nos_mass = btMax(btScalar(0), nos_mass - nos_consumed);
	}

	if (out_of_gas || rev_limit_exceeded || stalled)
		combustion_torque = 0.0;

	friction_torque = info.GetFrictionTorque(throttle_position, rpm);

	//try to model the static friction of the engine
	if (stalled)
		friction_torque *= 2;

	btScalar total_torque = combustion_torque + friction_torque + clutch_torque;
	shaft.applyMomentum(total_torque * dt);

	return clutch_torque;
}

void CarEngine::DebugPrint(std::ostream & out) const
{
	out << "---Engine---" << "\n";
	out << "Throttle position: " << throttle_position << "\n";
	out << "Combustion torque: " << combustion_torque << "\n";
	out << "Clutch torque: " << -clutch_torque << "\n";
	out << "Friction torque: " << friction_torque << "\n";
	out << "Total torque: " << GetTorque() << "\n";
	out << "RPM: " << GetRPM() << "\n";
	out << "Rev limit exceeded: " << rev_limit_exceeded << "\n";
	out << "Running: " << !stalled << "\n";
}

bool CarEngine::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, shaft.ang_velocity);
	_SERIALIZE_(s, throttle_position);
	_SERIALIZE_(s, clutch_torque);
	_SERIALIZE_(s, out_of_gas);
	_SERIALIZE_(s, rev_limit_exceeded);
	return true;
}
