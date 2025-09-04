// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FarmingTerrainSystem.generated.h"

class ATerrainActor;

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
    AFarmingTerrainSystem();

protected:
    virtual void BeginPlay() override;

public:
    // Grid configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 GridWidth = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 GridHeight = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    float TileSize = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    float TileSpacing = 10.f;

    // Terrain tile class
    UPROPERTY(EditDefaultsOnly, Category = "Spawning")
    TSubclassOf<ATerrainActor> TerrainActorClass;

    // Prototypes de atores para os estados
    UPROPERTY(EditDefaultsOnly, Category = "Terrain Actors")
    TSubclassOf<ATerrainActor> EmptyActorClass;

    UPROPERTY(EditDefaultsOnly, Category = "Terrain Actors")
    TSubclassOf<ATerrainActor> PlowedActorClass;

    UPROPERTY(EditDefaultsOnly, Category = "Terrain Actors")
    TSubclassOf<ATerrainActor> PlantedActorClass;

    UPROPERTY(EditDefaultsOnly, Category = "Terrain Actors")
    TSubclassOf<ATerrainActor> GrownActorClass;

    // Crop instancing (optional visual mesh)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instancing")
    UInstancedStaticMeshComponent* CropInstances;

    UPROPERTY(EditDefaultsOnly, Category = "Farming")
    TSubclassOf<AActor> CropActorClass;

    UPROPERTY(EditDefaultsOnly, Category = "Farming")
    UStaticMesh* CropMesh;

    // Internal data
    TMap<FIntPoint, ETileState> TileStates;
    TMap<FIntPoint, ATerrainActor*> TileToTerrainActorMap;
    TMap<int32, AActor*> PlantedCrops;
    TMap<int32, int32> TileToInstanceMap;

    // Core systems
    void InitializeGrid();
    void UpdateTileVisual(FIntPoint Tile, ETileState NewState);

    // Grid helpers
    int32 GetTileIndex(int32 X, int32 Y) const;
    FVector GetWorldLocationFromTile(int32 X, int32 Y) const;
    FVector GetCenteredWorldLocationFromTile(int32 X, int32 Y) const;
    FIntPoint GetTileFromWorldLocation(FVector WorldLocation) const;
    bool IsValidTile(int32 X, int32 Y) const;

    // Farming actions
    UFUNCTION(BlueprintCallable, Category = "Farming")
    void PlowTileAt(FVector WorldLocation);

    UFUNCTION(BlueprintCallable, Category = "Farming")
    void PlantSeedAt(FVector WorldLocation);

    UFUNCTION(BlueprintCallable, Category = "Farming")
    void GrowCrop(int32 X, int32 Y);

    UFUNCTION(BlueprintCallable, Category = "Farming")
    void HarvestCrop(int32 X, int32 Y);

    void UpdateCropInstance(int32 TileIndex, UStaticMesh* NewMesh);
    void RemoveCropInstance(int32 TileIndex);
};