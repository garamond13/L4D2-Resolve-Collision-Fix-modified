
#include "resolve_collision_tools.h"
#include "util_shared.h"
#include "NextBotGroundLocomotion.h"
#include "NextBotInterface.h"
#include "NextBotBodyInterface.h"

Vector NextBotGroundLocomotion::ResolveZombieCollisions(const Vector& pos)
{
	Vector adjustedNewPos = pos;

	CBaseEntity* me = collisiontools->MyInfectedPointer(m_nextBot);
	const float hullWidth = GetBot()->GetBodyInterface()->GetHullWidth();
	const float dt = GetUpdateInterval();

	// only avoid if we're actually trying to move somewhere, and are enraged
	if (me != NULL && !IsUsingLadder() && !IsClimbingOrJumping() && IsOnGround() && collisiontools->CBaseEntity_IsAlive(m_nextBot) && IsAttemptingToMove() /*&& GetBot()->GetBodyInterface()->IsArousal( IBody::INTENSE )*/)
	{
		const CUtlVector<CHandle<CBaseEntity>>& neighbors = collisiontools->Infected_GetNeighbors(me);
		Vector avoid = vec3_origin;
		float avoidWeight = 0.0f;
	
		FOR_EACH_VEC(neighbors, it)
		{
			CBaseEntity* them = gamehelpers->ReferenceToEntity(neighbors[it].GetEntryIndex());

			if (them)
			{
				Vector toThem = (collisiontools->CBaseEntity_GetAbsOrigin(them) - collisiontools->CBaseEntity_GetAbsOrigin(me));
				toThem.z = 0.0f;
	
				float range = toThem.NormalizeInPlace();

				if (range < hullWidth)
				{
					// these two infected are in contact
					collisiontools->CBaseEntity_Touch(me, them);
					
					// move out of contact
					float penetration = (hullWidth - range);
					float weight = 1.0f + (2.0f * penetration / hullWidth);
					
					avoid += -weight * toThem;
					avoidWeight += weight;
				}
			}
		}
	
		if (avoidWeight > 0.0f)
		{
			Vector collision = avoid / avoidWeight;
			collision *= (GetUpdateInterval() / 0.1f);
			adjustedNewPos += collision;
		}
	}

	return adjustedNewPos;
}