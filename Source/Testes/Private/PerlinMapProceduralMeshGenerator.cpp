// Fill out your copyright notice in the Description page of Project Settings.


#include "PerlinMapProceduralMeshGenerator.h"
#include "DrawDebugHelpers.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

// Sets default values
APerlinMapProceduralMeshGenerator::APerlinMapProceduralMeshGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
    RootComponent = ProceduralMesh;

    TerrainISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TerrainISM"));
    TerrainISM->SetupAttachment(RootComponent); // <-- ESSENCIAL

    TreeISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TreeISM"));
    TreeISM->SetupAttachment(RootComponent);

    WaterISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WaterISM"));
    WaterISM->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void APerlinMapProceduralMeshGenerator::BeginPlay()
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
void APerlinMapProceduralMeshGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APerlinMapProceduralMeshGenerator::GenerateMap()
{
    const int32 NumVertsX = MapWidth + 1;
    const int32 NumVertsY = MapHeight + 1;
    const float TileSize = 100.0f;

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;

    FRandomStream RandStream(Seed);

    // Gera vértices
    for (int32 Y = 0; Y < NumVertsY; ++Y)
    {
        for (int32 X = 0; X < NumVertsX; ++X)
        {
            float Noise = GeneratePerlinNoise(X, Y, RandStream);
            float Height = Noise * HeightMultiplier;
            Vertices.Add(FVector(X * TileSize, Y * TileSize, Height));
            Normals.Add(FVector::UpVector); // Placeholder
            UVs.Add(FVector2D((float)X / MapWidth, (float)Y / MapHeight));
            Tangents.Add(FProcMeshTangent(1, 0, 0));
        }
    }

    // Gera triângulos (2 por quad)
    for (int32 Y = 0; Y < MapHeight; ++Y)
    {
        for (int32 X = 0; X < MapWidth; ++X)
        {
            int32 i0 = Y * NumVertsX + X;
            int32 i1 = i0 + 1;
            int32 i2 = i0 + NumVertsX;
            int32 i3 = i2 + 1;

            // Triângulo 1
            Triangles.Add(i0);
            Triangles.Add(i2);
            Triangles.Add(i1);

            // Triângulo 2
            Triangles.Add(i1);
            Triangles.Add(i2);
            Triangles.Add(i3);
        }
    }

    ProceduralMesh->CreateMeshSection_LinearColor(
        0,
        Vertices,
        Triangles,
        Normals,
        UVs,
        TArray<FLinearColor>(),
        Tangents,
        true
    );

    // Depois de gerar vértices e triângulos
    TerrainVertices = Vertices;
    TerrainTriangles = Triangles;

    ProceduralMesh->CreateMeshSection_LinearColor(
        0,
        TerrainVertices,
        TerrainTriangles,
        Normals,
        UVs,
        TArray<FLinearColor>(),
        Tangents,
        true
    );

    ProceduralMesh->SetMaterial(0, TerrainMaterial); // Se quiser aplicar material
    ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ProceduralMesh->ContainsPhysicsTriMeshData(true);
}


float APerlinMapProceduralMeshGenerator::GeneratePerlinNoise(float X, float Y, FRandomStream& RandStream)
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

void APerlinMapProceduralMeshGenerator::ModifyTerrainAt(FVector WorldLocation, float Radius, float DeltaHeight)
{
    if (TerrainVertices.Num() == 0) return;

    FVector LocalLocation = ProceduralMesh->GetComponentTransform().InverseTransformPosition(WorldLocation);

    for (int32 i = 0; i < TerrainVertices.Num(); ++i)
    {
        float Dist = FVector2D::Distance(
            FVector2D(TerrainVertices[i].X, TerrainVertices[i].Y),
            FVector2D(LocalLocation.X, LocalLocation.Y)
        );

        if (Dist <= Radius)
        {
            // Suavemente afeta os vértices com base na distância
            float Falloff = 1.0f - (Dist / Radius);
            TerrainVertices[i].Z += DeltaHeight * Falloff;
        }
    }

    // Atualiza a mesh
    ProceduralMesh->UpdateMeshSection_LinearColor(
        0,
        TerrainVertices,
        TArray<FVector>(),     // Normals (opcional)
        TArray<FVector2D>(),   // UVs (opcional)
        TArray<FLinearColor>(),
        TArray<FProcMeshTangent>()
    );

    // Para cavar
    //ModifyTerrainAt(HitLocation, 200.0f, -100.0f);

    // Para adicionar terra
    //ModifyTerrainAt(HitLocation, 200.0f, +100.0f);
}