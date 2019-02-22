#include "stdafx_client.h"
#include "pragma/particlesystem/operators/c_particle_mod_velocity.h"
#include <mathutil/umath.h>
#include <pragma/math/vector/wvvector3.h>
#include <sharedutils/util_string.h>
#include <sharedutils/util.h>
#include <algorithm>

REGISTER_PARTICLE_OPERATOR(velocity,CParticleOperatorVelocity);

CParticleOperatorVelocity::CParticleOperatorVelocity(pragma::CParticleSystemComponent &pSystem,const std::unordered_map<std::string,std::string> &values)
	: CParticleOperator(pSystem,values)
{
	for(auto it=values.begin();it!=values.end();it++)
	{
		std::string key = it->first;
		StringToLower(key);
		if(key == "velocity")
			m_velocity = uvec::create(it->second);
	}
}
void CParticleOperatorVelocity::Simulate(CParticle &particle,double tDelta)
{
	Vector3 vel = particle.GetVelocity();
	vel += m_velocity *(float) tDelta;
	particle.SetVelocity(vel);
}
float CParticleOperatorVelocity::GetSpeed() const {return uvec::length(m_velocity);}
