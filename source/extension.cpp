#include "extension.h"
#include <CDetour/detours.h>
#include <compat_wrappers.h>
#include "resolve_collision.h"
#include "resolve_collision_tools.h"

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

DETOUR_DECL_MEMBER1(NextBotGroundLocomotion__ResolveZombieCollisions, Vector, const Vector&, pos)
{
	NextBotGroundLocomotion* groundLocomotion = (NextBotGroundLocomotion*)this;
	return groundLocomotion->ResolveZombieCollisions(pos);
}

bool SDKResolveCollision::SDK_OnLoad(char* error, size_t maxlen, bool late)
{
	if (!gameconfs->LoadGameConfigFile("l4d2_resolve_collision", &gpConfig, error, maxlen))
		return false;

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
