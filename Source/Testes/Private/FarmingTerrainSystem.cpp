// Fill out your copyright notice in the Description page of Project Settings.


#include "FarmingTerrainSystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include <ACropActor.h>

// Sets default values
AFarmingTerrainSystem::AFarmingTerrainSystem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    // Procedural Terrain Mesh
    TerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMesh"));
    TerrainMesh->SetupAttachment(RootComponent);
    TerrainMesh->bUseAsyncCooking = true;

    // Crops
    CropInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CropInstances"));
    CropInstances->SetupAttachment(RootComponent);

    //CropMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CropMesh"));
    //CropMesh->SetupAttachment(RootComponent);

    if (CropInstances)
    {
        CropInstances->SetFlags(RF_Transactional);
    };

}

// Called when the game starts or when spawned
void AFarmingTerrainSystem::BeginPlay()
{
	Super::BeginPlay();

    GenerateTerrain(); // ⬅️ Adicionado aqui

    if (GrownMesh)
    {
        CropInstances->SetStaticMesh(GrownMesh);
    }

    if (TerrainMaterial)
    {
        TerrainMesh->SetMaterial(0, TerrainMaterial);
    }

    InitializeGrid();
}

// Called every frame
void AFarmingTerrainSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AFarmingTerrainSystem::InitializeGrid()
{
    for (int32 Y = 0; Y < GridHeight; ++Y)
    {
        for (int32 X = 0; X < GridWidth; ++X)
        {
            FIntPoint Coord(X, Y);
            TileStates.Add(Coord, ETileState::Empty); // Estado inicial
        }
    }
}


void AFarmingTerrainSystem::GenerateTerrain()
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;

    const int32 NumVertsX = GridWidth + 1;
    const int32 NumVertsY = GridHeight + 1;

    // Cria os vértices
    for (int32 Y = 0; Y < NumVertsY; ++Y)
    {
        for (int32 X = 0; X < NumVertsX; ++X)
        {
            float Z = 0.0f; // terreno plano
            Vertices.Add(FVector(X * TileSize, Y * TileSize, Z));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D((float)X / GridWidth, (float)Y / GridHeight));
            Tangents.Add(FProcMeshTangent(1, 0, 0));
        }
    }

    // Cria os triângulos
    for (int32 Y = 0; Y < GridHeight; ++Y)
    {
        for (int32 X = 0; X < GridWidth; ++X)
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

    TerrainMesh->CreateMeshSection_LinearColor(
        0,
        Vertices,
        Triangles,
        Normals,
        UVs,
        TArray<FLinearColor>(),
        Tangents,
        true
    );

    TerrainMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}


int32 AFarmingTerrainSystem::GetTileIndex(int32 X, int32 Y) const
{
    return Y * GridWidth + X;
}

bool AFarmingTerrainSystem::IsValidTile(int32 X, int32 Y) const
{
    return X >= 0 && X < GridWidth && Y >= 0 && Y < GridHeight;
}

FVector AFarmingTerrainSystem::GetWorldLocationFromTile(int32 X, int32 Y) const
{
    return FVector(X * TileSize, Y * TileSize, 0.f);
}

FIntPoint AFarmingTerrainSystem::GetTileFromWorldLocation(FVector WorldLocation) const
{
    FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);

    int32 X = FMath::FloorToInt(Local.X / TileSize);
    int32 Y = FMath::FloorToInt(Local.Y / TileSize);

    return FIntPoint(X, Y);
}

void AFarmingTerrainSystem::PlowTileAt(FVector WorldLocation)
{
    FIntPoint Tile = GetTileFromWorldLocation(WorldLocation);

    if (Tile.X < 0 || Tile.X >= GridWidth || Tile.Y < 0 || Tile.Y >= GridHeight)
        return;

    ETileState& State = TileStates.FindOrAdd(Tile);

    if (State == ETileState::Empty)
    {
        State = ETileState::Plowed;
        UE_LOG(LogTemp, Log, TEXT("Tile (%d, %d) arado."), Tile.X, Tile.Y);
    }
}

void AFarmingTerrainSystem::PlantSeedAt(FVector WorldLocation)
{
    FIntPoint Tile = GetTileFromWorldLocation(WorldLocation);

    if (!IsValidTile(Tile.X, Tile.Y))
        return;

    ETileState& State = TileStates.FindOrAdd(Tile);

    if (State == ETileState::Plowed)
    {
        State = ETileState::Planted;

        FVector LocalLocation = FVector(
            (Tile.X + 0.5f) * TileSize,
            (Tile.Y + 0.5f) * TileSize,
            5.0f // Z offset opcional
        );

        FVector WorldPosition = GetActorTransform().TransformPosition(LocalLocation);

        if (CropActorClass)
        {
            FActorSpawnParameters SpawnParams;
            AActor* SpawnedCrop = GetWorld()->SpawnActor<AActor>(CropActorClass, WorldPosition, FRotator::ZeroRotator, SpawnParams);

            if (SpawnedCrop && CropMesh)
            {
                // Se o actor tiver método SetCropMesh (como no exemplo)
                if (AACropActor* Crop = Cast<AACropActor>(SpawnedCrop))
                {
                    Crop->SetCropMesh(CropMesh);
                }

                int32 TileIndex = GetTileIndex(Tile.X, Tile.Y);
                PlantedCrops.Add(TileIndex, SpawnedCrop);
            }
        }

        UE_LOG(LogTemp, Log, TEXT("Semente plantada no tile (%d, %d)."), Tile.X, Tile.Y);
    }
}


void AFarmingTerrainSystem::GrowCrop(int32 X, int32 Y)
{
    if (!IsValidTile(X, Y)) return;

    int32 Index = GetTileIndex(X, Y);
    if (TileStates[Index] == ETileState::Planted)
    {
        TileStates[Index] = ETileState::Grown;
        UpdateCropInstance(Index, GrownMesh);
    }
}

void AFarmingTerrainSystem::HarvestCrop(int32 X, int32 Y)
{
    if (!IsValidTile(X, Y)) return;

    int32 Index = GetTileIndex(X, Y);
    if (TileStates[Index] == ETileState::Grown)
    {
        TileStates[Index] = ETileState::Harvested;
        RemoveCropInstance(Index);
        // Aqui você pode dar recompensa ao jogador
    }
}

void AFarmingTerrainSystem::UpdateCropInstance(int32 TileIndex, UStaticMesh* NewMesh)
{
    if (!NewMesh || !CropInstances || !TileToInstanceMap.Contains(TileIndex)) return;

    int32 InstanceIndex = TileToInstanceMap[TileIndex];
    FTransform OldTransform;
    if (CropInstances->GetInstanceTransform(InstanceIndex, OldTransform, true))
    {
        CropInstances->RemoveInstance(InstanceIndex);
        CropInstances->SetStaticMesh(NewMesh);
        int32 NewInstance = CropInstances->AddInstance(OldTransform);
        TileToInstanceMap[TileIndex] = NewInstance;
    }
}

void AFarmingTerrainSystem::RemoveCropInstance(int32 TileIndex)
{
    if (PlantedCrops.Contains(TileIndex))
    {
        AActor* CropActor = PlantedCrops[TileIndex];
        if (CropActor && !CropActor->IsPendingKillPending())
        {
            CropActor->Destroy();
        }

        PlantedCrops.Remove(TileIndex);
    }
}

