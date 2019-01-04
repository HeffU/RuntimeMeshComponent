// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Components/MeshComponent.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshSection.h"
#include "RuntimeMeshGenericVertex.h"
#include "PhysicsEngine/ConvexElem.h"
#include "RuntimeMesh.h"
#include "RuntimeMeshComponent.generated.h"

/**
*	Component that allows you to specify custom triangle mesh geometry for rendering and collision.
*/
UCLASS(HideCategories = (Object, LOD), Meta = (BlueprintSpawnableComponent))
class RUNTIMEMESHCOMPONENT_API URuntimeMeshComponent : public UMeshComponent, public IInterface_CollisionDataProvider
{
	GENERATED_BODY()

private:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RuntimeMesh, Meta = (AllowPrivateAccess = "true", DisplayName = "Runtime Mesh"))
	URuntimeMesh* RuntimeMeshReference;

	void EnsureHasRuntimeMesh();




public:

	URuntimeMeshComponent(const FObjectInitializer& ObjectInitializer);

	/** Clears the geometry for ALL collision only sections */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	FORCEINLINE URuntimeMesh* GetRuntimeMesh() const
	{
		return RuntimeMeshReference;
	}

	/** Clears the geometry for ALL collision only sections */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	FORCEINLINE URuntimeMesh* GetOrCreateRuntimeMesh()
	{
		EnsureHasRuntimeMesh();

		return RuntimeMeshReference;
	}

	FORCEINLINE FRuntimeMeshDataRef GetRuntimeMeshData()
	{
		return GetRuntimeMesh() ? GetRuntimeMesh()->GetRuntimeMeshData() : FRuntimeMeshDataRef();
	}

	FORCEINLINE FRuntimeMeshDataRef GetOrCreateRuntimeMeshData()
	{
		return GetOrCreateRuntimeMesh()->GetRuntimeMeshData();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool ShouldSerializeMeshData()
	{
		return GetRuntimeMesh() ? GetRuntimeMesh()->ShouldSerializeMeshData() : false;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetShouldSerializeMeshData(bool bShouldSerialize)
	{
		GetOrCreateRuntimeMesh()->SetShouldSerializeMeshData(bShouldSerialize);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", Meta = (AllowPrivateAccess = "true", DisplayName = "Get Mobility"))
		ERuntimeMeshMobility GetRuntimeMeshMobility()
	{
		return Mobility == EComponentMobility::Movable ? ERuntimeMeshMobility::Movable :
			Mobility == EComponentMobility::Stationary ? ERuntimeMeshMobility::Stationary : ERuntimeMeshMobility::Static;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", Meta = (AllowPrivateAccess = "true", DisplayName = "Set Mobility"))
		void SetRuntimeMeshMobility(ERuntimeMeshMobility NewMobility)
	{
		Super::SetMobility(
			NewMobility == ERuntimeMeshMobility::Movable ? EComponentMobility::Movable :
			NewMobility == ERuntimeMeshMobility::Stationary ? EComponentMobility::Stationary : EComponentMobility::Static);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetRuntimeMesh(URuntimeMesh* NewMesh);

	// Wrapper functions --------------------------------------------------------------------------------------------------------------------------------

		/*
		 * Creates the mesh section if it doesn't exist,
		 * Otherwise update the section.
		 * Will automatically delete the section if there are no vertices given
		 */
		template<typename VertexType0, typename IndexType>
		void SetMeshSection(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<IndexType>& InTriangles, bool bCreateCollision = false,
			EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
		{
			auto Vertices0 = TArray<TArray<VertexType0>>({ InVertices0 });
			auto Triangles = TArray<TArray<IndexType>>({ InTriangles });
			SetMeshSection(SectionIndex, Vertices0, Triangles, bCreateCollision, UpdateFrequency, UpdateFlags);
		}


		/*
		 * Creates the mesh section if it doesn't exist,
		 * Otherwise update the section.
		 * Will automatically delete the section if there are no vertices given
		 */
		template<typename VertexType0, typename IndexType>
		void SetMeshSection(int32 SectionIndex, TArray<TArray<VertexType0>>& InVertices0, TArray<TArray<IndexType>>& InTriangles, bool bCreateCollision = false,
			EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None) 
		{
			check(IsInGameThread())
			if (GetOrCreateRuntimeMeshData()->DoesSectionExist(SectionIndex)) {
				if (InVertices0.Num() == 0) {
					GetOrCreateRuntimeMeshData()->ClearMeshSection(SectionIndex);
				}
				else {
					for (int32 LODIndex = 0; LODIndex < FMath::Min(InVertices0.Num(), InTriangles.Num()); LODIndex++)
					{
						GetOrCreateRuntimeMeshData()->UpdateMeshSection(SectionIndex, LODIndex, InVertices0[LODIndex], InTriangles[LODIndex], UpdateFlags);
					}
				}
			}
			else if (InVertices0.Num() != 0) {
				GetOrCreateRuntimeMeshData()->CreateMeshSection(SectionIndex, InVertices0, InTriangles, bCreateCollision, UpdateFrequency, UpdateFlags);
			}
		}

		/*
		 * Creates the mesh section if it doesn't exist,
		 * Otherwise update the section.
		 * Will automatically delete the section if there are no vertices given
		 */
		void SetMeshSection(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, bool bCreateCollision = false,
			EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
		{
			check(IsInGameThread())
			if (GetOrCreateRuntimeMeshData()->DoesSectionExist(SectionId)) {
				if (MeshData->NumIndices() == 0) {
					GetOrCreateRuntimeMeshData()->ClearMeshSection(SectionId);
				}
				else {
					GetOrCreateRuntimeMeshData()->UpdateMeshSection(SectionId, MeshData, UpdateFlags);
				}
			}
			else if (MeshData->NumIndices() != 0) {
				GetOrCreateRuntimeMeshData()->CreateMeshSection(SectionId, MeshData, bCreateCollision, UpdateFrequency, UpdateFlags);
			}
		}

		/*
		 * Creates the mesh section if it doesn't exist,
		 * Otherwise update the section.
		 * Will automatically delete the section if there are no vertices given
		 */
		void SetMeshSection(int32 SectionIndex, const TArray<TArray<FVector>>& Vertices, const TArray<TArray<int32>>& Triangles, const TArray<TArray<FVector>>& Normals,
			const TArray<TArray<FVector2D>>& UV0, const TArray<TArray<FColor>>& Colors, const TArray<TArray<FRuntimeMeshTangent>>& Tangents, bool bCreateCollision = false,
			EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None,
			bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true)
		{
			check(IsInGameThread())
			if (GetOrCreateRuntimeMeshData()->DoesSectionExist(SectionIndex)) {
				if (Vertices.Num() == 0) {
					GetOrCreateRuntimeMeshData()->ClearMeshSection(SectionIndex);
				}
				else {
					for (int32 LODIndex = 0; LODIndex < FMath::Min(Vertices.Num(), Triangles.Num()); LODIndex++)
					{
						GetOrCreateRuntimeMeshData()->UpdateMeshSection(SectionIndex, LODIndex, 
							Vertices[LODIndex], 
							Triangles[LODIndex], 
							Normals.IsValidIndex(LODIndex) ? Normals[LODIndex] : TArray<FVector>(),
							UV0.IsValidIndex(LODIndex) ? UV0[LODIndex] : TArray<FVector2D>(), 
							Colors.IsValidIndex(LODIndex) ? Colors[LODIndex] : TArray<FColor>(),
							Tangents.IsValidIndex(LODIndex) ? Tangents[LODIndex] : TArray<FRuntimeMeshTangent>(),
							UpdateFlags);
					}
				}
			}
			else if (Vertices.Num() != 0) {
				GetOrCreateRuntimeMeshData()->CreateMeshSection(SectionIndex, 0, Vertices[0], Triangles[0], Normals[0], UV0[0], Colors[0], Tangents[0], bCreateCollision,
					UpdateFrequency, UpdateFlags, bUseHighPrecisionTangents, bUseHighPrecisionUVs);

				for (int32 LODIndex = 1; LODIndex < FMath::Min(Vertices.Num(), Triangles.Num()); LODIndex++)
				{
					GetOrCreateRuntimeMeshData()->UpdateMeshSection(SectionIndex, LODIndex,
						Vertices[LODIndex],
						Triangles[LODIndex],
						Normals.IsValidIndex(LODIndex) ? Normals[LODIndex] : TArray<FVector>(),
						UV0.IsValidIndex(LODIndex) ? UV0[LODIndex] : TArray<FVector2D>(),
						Colors.IsValidIndex(LODIndex) ? Colors[LODIndex] : TArray<FColor>(),
						Tangents.IsValidIndex(LODIndex) ? Tangents[LODIndex] : TArray<FRuntimeMeshTangent>(),
						UpdateFlags);
				}
			}
		}

		/*
		 * Creates the mesh section if it doesn't exist,
		 * Otherwise update the section.
		 * Will automatically delete the section if there are no vertices given
		 */
		UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Set Mesh Section", AutoCreateRefTerm = "Normals,Tangents,UV0,UV1,Colors"))
			void SetMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
				const TArray<FRuntimeMeshTangent>& Tangents, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& Colors, int32 LODIndex = 0,
				bool bCreateCollision = false, bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false,
				EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true)
		{
			check(IsInGameThread())
			if (GetOrCreateRuntimeMeshData()->DoesSectionExist(SectionIndex)) {
				if (Vertices.Num() == 0) {
					GetOrCreateRuntimeMeshData()->ClearMeshSection(SectionIndex);
				}
				else {
					GetOrCreateRuntimeMeshData()->UpdateMeshSection_Blueprint(SectionIndex, Vertices, Triangles, Normals, Tangents, UV0, UV1, Colors,
						bCalculateNormalTangent, bShouldCreateHardTangents, bGenerateTessellationTriangles, LODIndex);
				}
			}
			else if (Vertices.Num() != 0) {
				GetOrCreateRuntimeMeshData()->CreateMeshSection_Blueprint(SectionIndex, Vertices, Triangles, Normals, Tangents, UV0, UV1, Colors, bCreateCollision,
					bCalculateNormalTangent, bShouldCreateHardTangents, bGenerateTessellationTriangles, UpdateFrequency, bUseHighPrecisionTangents, bUseHighPrecisionUVs, LODIndex);
			}
		}

		/*
		 * Creates the mesh section if it doesn't exist,
		 * Otherwise update the section.
		 * Will automatically delete the section if there are no vertices given
		 */
		UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Set Mesh Section Packed", AutoCreateRefTerm = "Normals,Tangents,UV0,UV1,Colors"))
			void SetMeshSectionPacked_Blueprint(int32 SectionIndex, const TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, const TArray<int32>& Triangles, int32 LODIndex = 0,
				bool bCreateCollision = false, bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
				bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true)
		{
			check(IsInGameThread())
			if (GetOrCreateRuntimeMeshData()->DoesSectionExist(SectionIndex)) {
				if (Vertices.Num() == 0) {
					GetOrCreateRuntimeMeshData()->ClearMeshSection(SectionIndex);
				}
				else {
					GetOrCreateRuntimeMeshData()->UpdateMeshSectionPacked_Blueprint(SectionIndex, Vertices, Triangles, bCalculateNormalTangent, bShouldCreateHardTangents,
						bGenerateTessellationTriangles, LODIndex);
				}
			}
			else if (Vertices.Num() != 0) {
				GetOrCreateRuntimeMeshData()->CreateMeshSectionPacked_Blueprint(SectionIndex, Vertices, Triangles, bCreateCollision, bCalculateNormalTangent, bShouldCreateHardTangents,
					bGenerateTessellationTriangles, UpdateFrequency, bUseHighPrecisionTangents, bUseHighPrecisionUVs, LODIndex);
			}
		}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Properties of the Runtime Mesh --------------------------------------------------------------------------------------------------------------------------------


private:

	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual bool IsSupportedForNetworking() const override
	{
		return true;
	}
	//~ Begin USceneComponent Interface.

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual class UBodySetup* GetBodySetup() override;

public:

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 GetSectionIdFromCollisionFaceIndex(int32 FaceIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void GetSectionIdAndFaceIdFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex, int32& SectionFaceIndex) const;

	virtual UMaterialInterface* GetMaterialFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex) const override;
	//~ End UPrimitiveComponent Interface.

public:
	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	virtual UMaterialInterface* GetOverrideMaterial(int32 ElementIndex) const;
	//~ End UMeshComponent Interface.

private:

	/* Serializes this component */
	virtual void Serialize(FArchive& Ar) override;


	/* Does post load fixups */
	virtual void PostLoad() override;



	/** Called by URuntimeMesh any time it has new collision data that we should use */
	void NewCollisionMeshReceived();
	void NewBoundsReceived();
	void ForceProxyRecreate();

	void SendSectionCreation(int32 SectionIndex);
	void SendSectionPropertiesUpdate(int32 SectionIndex);

	// This collision setup is only to support older engine versions where the BodySetup being owned by a non UActorComponent breaks runtime cooking

	/** Collision data */
	UPROPERTY(Instanced)
	UBodySetup* BodySetup;

	/** Queue of pending collision cooks */
	UPROPERTY(Transient)
	TArray<UBodySetup*> AsyncBodySetupQueue;

	//~ Begin Interface_CollisionDataProvider Interface
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 22
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
	virtual bool WantsNegXTriMesh() override { return false; }
	//~ End Interface_CollisionDataProvider Interface
#endif

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 21
	UBodySetup* CreateNewBodySetup();
	void FinishPhysicsAsyncCook(UBodySetup* FinishedBodySetup);

	void UpdateCollision(bool bForceCookNow);
#endif



	friend class URuntimeMesh;
	friend class FRuntimeMeshComponentSceneProxy;
	friend class FRuntimeMeshComponentLegacySerialization;
};
