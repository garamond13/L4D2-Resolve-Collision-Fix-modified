#include "extension.h"
#include <CDetour/detours.h>
#include <compat_wrappers.h>
#include "resolve_collision_tools.h"
#include "util_shared.h"
#include "NextBotGroundLocomotion.h"
#include "NextBotInterface.h"
#include "NextBotBodyInterface.h"

SDKResolveCollision g_sdkResolveCollision;
SMEXT_LINK(&g_sdkResolveCollision);

IPhysics* iphysics = nullptr;
IGameConfig* gpConfig = nullptr;
CGlobalVars* gpGlobals = nullptr; 
ICvar* icvar = nullptr;
IEngineTrace* enginetrace = nullptr;
IStaticPropMgrServer* staticpropmgr = nullptr;
ISDKHooks* g_pSDKHooks = nullptr;
CDetour* g_pResolveZombieCollisionDetour = nullptr;

Vector NextBotGroundLocomotion::ResolveZombieCollisions(const Vector& pos)
{
	Vector adjustedNewPos = pos;
	CBaseEntity* me = collisiontools->MyInfectedPointer(m_nextBot);
	const float hullWidth = GetBot()->GetBodyInterface()->GetHullWidth();

	// only avoid if we're actually trying to move somewhere, and are enraged
	if (me && !IsUsingLadder() && !IsClimbingOrJumping() && IsOnGround() && collisiontools->CBaseEntity_IsAlive(m_nextBot) && IsAttemptingToMove() /*&& GetBot()->GetBodyInterface()->IsArousal( IBody::INTENSE )*/) {
		const CUtlVector<CHandle<CBaseEntity>>& neighbors = collisiontools->Infected_GetNeighbors(me);
		Vector avoid = vec3_origin;
		float avoidWeight = 0.0f;
		FOR_EACH_VEC(neighbors, it) {
			CBaseEntity* them = gamehelpers->ReferenceToEntity(neighbors[it].GetEntryIndex());
			if (them) {
				Vector toThem = (collisiontools->CBaseEntity_GetAbsOrigin(them) - collisiontools->CBaseEntity_GetAbsOrigin(me));
				toThem.z = 0.0f;
				float range = toThem.NormalizeInPlace();
				if (range < hullWidth) {

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
		if (avoidWeight > 0.0f) {
			Vector collision = avoid / avoidWeight;
			collision *= GetUpdateInterval() / 0.1f;
			adjustedNewPos += collision;
		}
	}

	return adjustedNewPos;
}

DETOUR_DECL_MEMBER1(NextBotGroundLocomotion__ResolveZombieCollisions, Vector, const Vector&, pos)
{
	NextBotGroundLocomotion* groundLocomotion = (NextBotGroundLocomotion*)this;
	return groundLocomotion->ResolveZombieCollisions(pos);
}

bool SDKResolveCollision::SDK_OnLoad(char* error, size_t maxlen, bool late)
{
	if (!gameconfs->LoadGameConfigFile("l4d2_resolve_collision", &gpConfig, error, maxlen)) {
		return false;
	}
	if (!collisiontools->Initialize(gpConfig)) {
		V_snprintf(error, maxlen, "Failed to initialize ResolveCollision tools");
		return false;
	}
	CDetourManager::Init(g_pSM->GetScriptingEngine(), gpConfig);
	g_pResolveZombieCollisionDetour = DETOUR_CREATE_MEMBER(NextBotGroundLocomotion__ResolveZombieCollisions, "NextBotGroundLocomotion::ResolveZombieCollisions");
	g_pResolveZombieCollisionDetour->EnableDetour();
	return true;
}

bool SDKResolveCollision::SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	GET_V_IFACE_ANY(GetEngineFactory, staticpropmgr, IStaticPropMgrServer, INTERFACEVERSION_STATICPROPMGR_SERVER);
	GET_V_IFACE_ANY(GetEngineFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetPhysicsFactory, iphysics, IPhysics, VPHYSICS_INTERFACE_VERSION);
	g_pCVar = icvar;
	CONVAR_REGISTER(this);
	gpGlobals = ismm->GetCGlobals();
	return true;
};

void SDKResolveCollision::SDK_OnUnload()
{
	if (g_pResolveZombieCollisionDetour) {
		g_pResolveZombieCollisionDetour->Destroy();
		g_pResolveZombieCollisionDetour = nullptr;
	}
}

bool SDKResolveCollision::RegisterConCommandBase(ConCommandBase* command)
{
	return META_REGCVAR(command);
}
