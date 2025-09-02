// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "PerlinMapProceduralMeshGenerator.generated.h"

UCLASS()
class TESTES_API APerlinMapProceduralMeshGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APerlinMapProceduralMeshGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, Category = "Map Settings")
    int32 MapWidth = 100;

    UPROPERTY(EditAnywhere, Category = "Map Settings")
    int32 MapHeight = 100;

    UPROPERTY(EditAnywhere, Category = "Map Settings")
    float NoiseScale = 50.0f;

    UPROPERTY(EditAnywhere, Category = "Map Settings")
    float HeightMultiplier = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Octaves = 4;

    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    float Persistence = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    float Lacunarity = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed = 1337;

    UPROPERTY(EditAnywhere, Category = "Mesh")
    UStaticMesh* CubeMesh;

    UPROPERTY(EditAnywhere, Category = "Mesh")
    UMaterialInterface* CubeMaterial;

    // Malhas por bioma
    UPROPERTY(EditAnywhere, Category = "Meshes")
    UStaticMesh* TerrainMesh;

    UPROPERTY(EditAnywhere, Category = "Meshes")
    UStaticMesh* TreeMesh;

    UPROPERTY(EditAnywhere, Category = "Meshes")
    UStaticMesh* WaterMesh;

    // Materiais opcionais
    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* WaterMaterial;

    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* TerrainMaterial;

    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* TreeMaterial;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UProceduralMeshComponent* ProceduralMesh;

    UPROPERTY()
    TArray<FVector> TerrainVertices;

    UPROPERTY()
    TArray<int32> TerrainTriangles;

    UFUNCTION(BlueprintCallable, Category = "Terrain")
    void ModifyTerrainAt(FVector WorldLocation, float Radius, float DeltaHeight);

    UFUNCTION(BlueprintCallable, Category = "Terrain")
    void LevelTerrainAt(FVector WorldLocation, float Radius, float TargetHeight);

    UFUNCTION(BlueprintCallable, Category = "Terrain")
    void AddTributaryAt(FVector StartLocation);

    UFUNCTION(BlueprintCallable, Category = "Terrain")
    void SimulateErosion(int32 NumIterations, float RainAmount, float ErosionStrength);

    UFUNCTION(BlueprintCallable)
    void SimulateErosionAt(FVector WorldLocation, float Radius, int32 NumIterations, float RainAmount, float ErosionStrength);



    // ISMs separados
    UInstancedStaticMeshComponent* TerrainISM;
    UInstancedStaticMeshComponent* TreeISM;
    UInstancedStaticMeshComponent* WaterISM;

    TArray<FVector2D> MainRiverPath;
    TArray<TArray<FVector2D>> AllRiverPaths;


private:
    UInstancedStaticMeshComponent* InstancedMeshComp;

    void GenerateMap();
    float GeneratePerlinNoise(float X, float Y, FRandomStream& RandStream);
    void CarveRiver(const FVector2D& Start, const FVector2D& End, float Width, float Depth);
    TArray<FVector2D> GenerateCurvedRiverPath(int32 NumPoints, FVector2D Start, FVector2D End, float Amplitude, float Frequency);
    void CarveCurvedRiver(const TArray<FVector2D>& RiverPath, float Width, float Depth);



};
