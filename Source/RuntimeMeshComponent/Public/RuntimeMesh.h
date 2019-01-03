// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshSection.h"
#include "RuntimeMeshData.h"
#include "RuntimeMeshBlueprint.h"
#include "RuntimeMeshCollision.h"
#include "RuntimeMeshBlueprintMeshBuilder.h"
#include "RuntimeMesh.generated.h"

class UBodySetup;
class URuntimeMesh;
class URuntimeMeshComponent;




/*
*	This tick function is used to drive the collision cooker.
*	It is enabled for one frame when we need to update collision.
*	This keeps from cooking on each individual create/update section as the original PMC did
*/
struct FRuntimeMeshCollisionCookTickObject : FTickableGameObject
{
	TWeakObjectPtr<URuntimeMesh> Owner;

	FRuntimeMeshCollisionCookTickObject(TWeakObjectPtr<URuntimeMesh> InOwner) : Owner(InOwner) {}
	virtual void Tick(float DeltaTime);
	virtual bool IsTickable() const;
	virtual bool IsTickableInEditor() const { return false; }
	virtual TStatId GetStatId() const;

	virtual UWorld* GetTickableGameObjectWorld() const;
};


/**
*	Delegate for when the collision was updated.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRuntimeMeshCollisionUpdatedDelegate);


UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMesh : public UObject, public IInterface_CollisionDataProvider
{
	GENERATED_UCLASS_BODY()

private:
	/** Reference to the underlying data object */
	FRuntimeMeshDataRef Data;

	/** Materials for this Runtime Mesh */
	UPROPERTY(EditAnywhere, Category = "RuntimeMesh")
	TArray<UMaterialInterface*> Materials;


	/** Do we need to update our collision? */
	bool bCollisionIsDirty;

	/** Object used to tick the collision cooking at the end of the frame */
	TUniquePtr<FRuntimeMeshCollisionCookTickObject> CookTickObject;

	/** All RuntimeMeshComponents linked to this mesh. Used to alert the components of changes */
	TArray<TWeakObjectPtr<URuntimeMeshComponent>> LinkedComponents;

	/**
	*	Controls whether the complex (Per poly) geometry should be treated as 'simple' collision.
	*	Should be set to false if this component is going to be given simple collision and simulated.
	*/
	UPROPERTY(EditAnywhere, Category = "RuntimeMesh")
	bool bUseComplexAsSimpleCollision;

	/**
	*	Controls whether the physics cooking is done in parallel. This will increase throughput in
	*	multiple RMC scenarios, and keep from blocking the game thread, but when the collision becomes queryable
	*	is non-deterministic. See callback event for notification on collision updated.
	*/
	UPROPERTY(EditAnywhere, Category = "RuntimeMesh")
	bool bUseAsyncCooking;

	/**
	*	Controls whether the mesh data should be serialized with the component.
	*/
	UPROPERTY(EditAnywhere, Category = "RuntimeMesh")
	bool bShouldSerializeMeshData;

	/** Collision cooking configuration. Prefer runtime performance or cooktime speed */
	UPROPERTY(EditAnywhere, Category = "RuntimeMesh")
	ERuntimeMeshCollisionCookingMode CollisionMode;

	/** Collision data */
	UPROPERTY(Instanced)
	UBodySetup* BodySetup;

	/** Queue of pending collision cooks */
	UPROPERTY(Transient)
	TArray<UBodySetup*> AsyncBodySetupQueue;

public:

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool ShouldSerializeMeshData() 
	{
		return bShouldSerializeMeshData;			
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetShouldSerializeMeshData(bool bShouldSerialize)
	{
		bShouldSerializeMeshData = bShouldSerialize;
	}

	/** Event called when the collision has finished updated, this works both with standard following frame synchronous updates, as well as async updates */
	UPROPERTY(BlueprintAssignable, Category = "Components|RuntimeMesh")
	FRuntimeMeshCollisionUpdatedDelegate CollisionUpdated;

	/** Gets the internal mesh data */
	FRuntimeMeshDataRef GetRuntimeMeshData() const
	{
		//check(IsInGameThread());
		return Data;
	}

public:

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetSectionMaterial(int32 SectionId, UMaterialInterface* Material)
	{
		check(IsInGameThread());
		if (SectionId >= Materials.Num())
		{
			Materials.SetNum(SectionId + 1);
		}

		Materials[SectionId] = Material;

		if (Data->DoesSectionExist(SectionId))
		if (Data->DoesSectionExist(SectionId))
		{
			ForceProxyRecreate();
		}
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	UMaterialInterface* GetSectionMaterial(int32 SectionId)
	{
		check(IsInGameThread());
		if (Materials.IsValidIndex(SectionId))
		{
			return Materials[SectionId];
		}
		return nullptr;
	}

	TArray<UMaterialInterface*> GetMaterials()
	{
		check(IsInGameThread());
		return Materials;
	}


	/** Runs any pending collision cook (Not required to call this. This is only if you need to make sure all changes are cooked before doing something)*/
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void CookCollisionNow();

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionUseComplexAsSimple(bool bNewValue)
	{
		check(IsInGameThread());
		bUseComplexAsSimpleCollision = bNewValue;
		MarkCollisionDirty();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsCollisionUsingComplexAsSimple()
	{
		check(IsInGameThread());
		return bUseComplexAsSimpleCollision;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionUseAsyncCooking(bool bNewValue)
	{
		check(IsInGameThread());
		bUseAsyncCooking = bNewValue;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsCollisionUsingAsyncCooking()
	{
		check(IsInGameThread());
		return bUseAsyncCooking;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionMode(ERuntimeMeshCollisionCookingMode NewMode)
	{
		check(IsInGameThread());
		CollisionMode = NewMode;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	ERuntimeMeshCollisionCookingMode GetCollisionMode() const
	{
		check(IsInGameThread());
		return CollisionMode;
	}

	UBodySetup* GetBodySetup() 
	{ 
		check(IsInGameThread());
		return BodySetup; 
	}

private:

	void Initialize() { GetRuntimeMeshData()->Initialize(); }
	virtual void MarkChanged() 
	{ 
#if WITH_EDITOR
		Modify(true);
		PostEditChange();
#endif
	}

	void RegisterLinkedComponent(URuntimeMeshComponent* NewComponent);
	void UnRegisterLinkedComponent(URuntimeMeshComponent* ComponentToRemove);

	template<typename Function>
	void DoForAllLinkedComponents(Function Func)
	{
		bool bShouldPurge = false;
		for (TWeakObjectPtr<URuntimeMeshComponent> MeshReference : LinkedComponents)
		{
			if (URuntimeMeshComponent* Mesh = MeshReference.Get())
			{
				Func(Mesh);
			}
			else
			{
				bShouldPurge = true;
			}
		}
		if (bShouldPurge)
		{
			LinkedComponents = LinkedComponents.FilterByPredicate([](const TWeakObjectPtr<URuntimeMeshComponent>& MeshReference)
			{
				return MeshReference.IsValid();
			});
		}
	}


	void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const
	{
		OutMaterials.Append(Materials.FilterByPredicate([](UMaterialInterface* Mat) -> bool { return Mat != nullptr; }));
	}

	//~ Begin Interface_CollisionDataProvider Interface
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
	virtual bool WantsNegXTriMesh() override { return false; }
	//~ End Interface_CollisionDataProvider Interface

	virtual void Serialize(FArchive& Ar) override;
	void PostLoad();


public:
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	UMaterialInterface* GetMaterialFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 GetSectionIdFromCollisionFaceIndex(int32 FaceIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void GetSectionIdAndFaceIndexFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex, int32& SectionFaceIndex) const;


private:
	/** Triggers a rebuild of the collision data on the next tick */
	void MarkCollisionDirty();

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 21
	/** Helper to create new body setup objects */
	UBodySetup* CreateNewBodySetup();
#endif
	/** Copies the convex element geometry to a supplied body setup */
	void CopyCollisionElementsToBodySetup(UBodySetup* Setup);
	/** Sets all basic configuration on body setup */
	void SetBasicBodySetupParameters(UBodySetup* Setup);
	/** Mark collision data as dirty, and re-create on instance if necessary */
	void UpdateCollision(bool bForceCookNow = false);
	/** Once async physics cook is done, create needed state, and then call the user event */
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 21
	void FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup);
	/** Runs all post cook tasks like alerting the user event and alerting linked components */
	void FinalizeNewCookedData();
#endif

	void UpdateLocalBounds();

	void ForceProxyRecreate();

	void SendSectionCreation(int32 SectionIndex);

	void SendSectionPropertiesUpdate(int32 SectionIndex);


	friend class FRuntimeMeshData;
	friend class URuntimeMeshComponent;
	friend class FRuntimeMeshComponentSceneProxy;
	friend struct FRuntimeMeshCollisionCookTickObject;
};

