// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "FarmingTerrainSystem.generated.h"

UENUM(BlueprintType)
enum class ETileState : uint8
{
	Empty      UMETA(DisplayName = "Empty"),
	Plowed     UMETA(DisplayName = "Plowed"),
	Planted    UMETA(DisplayName = "Planted"),
	Grown      UMETA(DisplayName = "Grown"),
	Harvested  UMETA(DisplayName = "Harvested")
};

UCLASS()
class TESTES_API AFarmingTerrainSystem : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFarmingTerrainSystem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    void InitializeGrid();

    void GenerateTerrain();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 GridWidth = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 GridHeight = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    float TileSize = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instancing")
    UInstancedStaticMeshComponent* CropInstances;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meshes")
    UStaticMesh* GrownMesh;

    // Terrain Mesh
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
    UProceduralMeshComponent* TerrainMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Farming")
    UMaterialInterface* TerrainMaterial;

    UPROPERTY()
    TMap<FIntPoint, ETileState> TileStates;

    UPROPERTY(EditDefaultsOnly, Category = "Farming")
    TSubclassOf<AActor> CropActorClass;

    UPROPERTY(EditDefaultsOnly, Category = "Farming")
    UStaticMesh* CropMesh;

    // Mapeamento de plantações por tile
    UPROPERTY()
    TMap<int32, AActor*> PlantedCrops;

    //TArray<ETileState> TileStates;

    // Grid utils
    int32 GetTileIndex(int32 X, int32 Y) const;
    FVector GetWorldLocationFromTile(int32 X, int32 Y) const;
    bool IsValidTile(int32 X, int32 Y) const;

    // Farming actions
    UFUNCTION(BlueprintCallable, Category = "Farming")
    void PlowTileAt(FVector WorldLocation);

    UFUNCTION(BlueprintCallable, Category = "Farming")
    void PlantSeedAt(FVector WorldLocation);

    UFUNCTION(BlueprintCallable)
    void GrowCrop(int32 X, int32 Y);

    UFUNCTION(BlueprintCallable)
    void HarvestCrop(int32 X, int32 Y);

    void UpdateCropInstance(int32 Index, UStaticMesh* NewMesh);
    void RemoveCropInstance(int32 Index);
    FIntPoint GetTileFromWorldLocation(FVector WorldLocation) const;


private:
    TMap<int32, int32> TileToInstanceMap;

};
