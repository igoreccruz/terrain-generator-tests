// Fill out your copyright notice in the Description page of Project Settings.


#include "PerlinMapGenerator.h"
#include "DrawDebugHelpers.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

// Sets default values
APerlinMapGenerator::APerlinMapGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;

    TerrainISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TerrainISM"));
    TreeISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TreeISM"));
    WaterISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WaterISM"));

    RootComponent = TerrainISM;

    TreeISM->SetupAttachment(RootComponent);
    WaterISM->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void APerlinMapGenerator::BeginPlay()
{
    Super::BeginPlay();

    if (!TerrainMesh || !TreeMesh || !WaterMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("Um ou mais meshes não foram definidos!"));
        return;
    }

    TerrainISM->SetStaticMesh(TerrainMesh);
    TreeISM->SetStaticMesh(TreeMesh);
    WaterISM->SetStaticMesh(WaterMesh);

    if (TerrainMaterial) TerrainISM->SetMaterial(0, TerrainMaterial);
    if (TreeMaterial) TreeISM->SetMaterial(0, TreeMaterial);
    if (WaterMaterial) WaterISM->SetMaterial(0, WaterMaterial);

    GenerateMap();
	
}

// Called every frame
void APerlinMapGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APerlinMapGenerator::GenerateMap()
{
    FRandomStream RandStream(Seed);
    FVector Origin = GetActorLocation();

    const float TileSize = 100.0f;
    const float WaterHeight = 0.3f;

    for (int32 X = 0; X < MapWidth; ++X)
    {
        for (int32 Y = 0; Y < MapHeight; ++Y)
        {
            float NoiseValue = GeneratePerlinNoise((float)X, (float)Y, RandStream); // [0,1]
            float Height = NoiseValue * HeightMultiplier;

            FVector TileLocation = Origin + FVector(X * TileSize, Y * TileSize, Height * 0.5f);
            FVector TileScale(1.0f, 1.0f, Height / TileSize);

            // Instancia o bloco de terreno
            TerrainISM->AddInstance(FTransform(FRotator::ZeroRotator, TileLocation, TileScale));

            // Água
            if (NoiseValue < WaterHeight)
            {
                FVector WaterLocation = Origin + FVector(X * TileSize, Y * TileSize, WaterHeight * HeightMultiplier);
                WaterISM->AddInstance(FTransform(FRotator::ZeroRotator, WaterLocation, FVector(1, 1, 0.05f)));
            }
            // Vegetação (ex: floresta)
            else if (NoiseValue >= 0.4f && NoiseValue < 0.6f)
            {
                if (RandStream.FRand() < 0.2f) // 20% chance de ter árvore
                {
                    FVector TreeLocation = Origin + FVector(X * TileSize, Y * TileSize, Height + 50.0f);
                    TreeISM->AddInstance(FTransform(FRotator::ZeroRotator, TreeLocation, FVector(1.0f)));
                }
            }
        }
    }
}


float APerlinMapGenerator::GeneratePerlinNoise(float X, float Y, FRandomStream& RandStream)
{
    float Total = 0.0f;
    float Frequency = 6.5f;
    float Amplitude = 100.0f;
    float MaxValue = 0.0f;

    for (int32 i = 0; i < Octaves; ++i)
    {
        float SampleX = (X / NoiseScale) * Frequency;
        float SampleY = (Y / NoiseScale) * Frequency;

        // Ruído Perlin normalizado [0,1]
        float Noise = FMath::PerlinNoise2D(FVector2D(SampleX, SampleY)) * 0.5f + 0.5f;

        Total += Noise * Amplitude;

        MaxValue += Amplitude;
        Amplitude *= Persistence;
        Frequency *= Lacunarity;
    }

    return Total / MaxValue; // Normaliza para [0,1]
}
