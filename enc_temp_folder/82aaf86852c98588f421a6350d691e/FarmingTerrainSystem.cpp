#include "FarmingTerrainSystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ACropActor.h"
#include "TerrainActor.h"

// Sets default values
AFarmingTerrainSystem::AFarmingTerrainSystem()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    CropInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CropInstances"));
    CropInstances->SetupAttachment(RootComponent);
    CropInstances->SetFlags(RF_Transactional);
}

void AFarmingTerrainSystem::BeginPlay()
{
    Super::BeginPlay();
    InitializeGrid();
}

void AFarmingTerrainSystem::InitializeGrid()
{
    for (int32 Y = 0; Y < GridHeight; ++Y)
    {
        for (int32 X = 0; X < GridWidth; ++X)
        {
            FIntPoint Coord(X, Y);
            TileStates.Add(Coord, ETileState::Empty);

            if (TerrainActorClass)
            {
                FVector WorldLocation = GetCenteredWorldLocationFromTile(X, Y);

                FActorSpawnParameters SpawnParams;
                ATerrainActor* TileActor = GetWorld()->SpawnActor<ATerrainActor>(
                    TerrainActorClass,
                    WorldLocation,
                    FRotator::ZeroRotator,
                    SpawnParams
                );

                if (TileActor)
                {
                    TileActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
                    TileToTerrainActorMap.Add(Coord, TileActor);
                }
            }
        }
    }
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
    float PosX = X * (TileSize + TileSpacing);
    float PosY = Y * (TileSize + TileSpacing);
    return FVector(PosX, PosY, 0.f);
}

FVector AFarmingTerrainSystem::GetCenteredWorldLocationFromTile(int32 X, int32 Y) const
{
    float PosX = (X + 0.5f) * (TileSize + TileSpacing);
    float PosY = (Y + 0.5f) * (TileSize + TileSpacing);
    return GetActorTransform().TransformPosition(FVector(PosX, PosY, 0.f));
}

FIntPoint AFarmingTerrainSystem::GetTileFromWorldLocation(FVector WorldLocation) const
{
    FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
    int32 X = FMath::FloorToInt(Local.X / (TileSize + TileSpacing));
    int32 Y = FMath::FloorToInt(Local.Y / (TileSize + TileSpacing));
    return FIntPoint(X, Y);
}

void AFarmingTerrainSystem::PlowTileAt(FVector WorldLocation)
{
    FIntPoint Tile = GetTileFromWorldLocation(WorldLocation);

    if (!IsValidTile(Tile.X, Tile.Y)) return;

    ETileState& State = TileStates.FindOrAdd(Tile);

    if (State == ETileState::Empty)
    {
        State = ETileState::Plowed;
        UpdateTileVisual(Tile, ETileState::Plowed);
        UE_LOG(LogTemp, Log, TEXT("Tile (%d, %d) arado."), Tile.X, Tile.Y);
    }
}

void AFarmingTerrainSystem::PlantSeedAt(FVector WorldLocation)
{
    FIntPoint Tile = GetTileFromWorldLocation(WorldLocation);

    if (!IsValidTile(Tile.X, Tile.Y)) return;

    ETileState& State = TileStates.FindOrAdd(Tile);

    if (State == ETileState::Plowed)
    {
        State = ETileState::Planted;
        UpdateTileVisual(Tile, ETileState::Planted);

        FVector WorldPosition = GetCenteredWorldLocationFromTile(Tile.X, Tile.Y) + FVector(0.f, 0.f, 5.f); // pequeno offset Z

        if (CropActorClass)
        {
            FActorSpawnParameters SpawnParams;
            AActor* SpawnedCrop = GetWorld()->SpawnActor<AActor>(CropActorClass, WorldPosition, FRotator::ZeroRotator, SpawnParams);

            if (SpawnedCrop && CropMesh)
            {
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
    FIntPoint Tile(X, Y);

    if (TileStates[Tile] == ETileState::Planted)
    {
        TileStates[Tile] = ETileState::Grown;
        UpdateTileVisual(Tile, ETileState::Grown);
    }
}

void AFarmingTerrainSystem::HarvestCrop(int32 X, int32 Y)
{
    if (!IsValidTile(X, Y)) return;

    int32 Index = GetTileIndex(X, Y);
    FIntPoint Tile(X, Y);

    if (TileStates[Tile] == ETileState::Grown)
    {
        TileStates[Tile] = ETileState::Empty;
        UpdateTileVisual(Tile, ETileState::Empty);
        RemoveCropInstance(Index);
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

void AFarmingTerrainSystem::UpdateTileVisual(FIntPoint Tile, ETileState NewState)
{
    // Remove antigo
    if (TileToTerrainActorMap.Contains(Tile))
    {
        ATerrainActor* OldActor = TileToTerrainActorMap[Tile];
        if (OldActor && !OldActor->IsPendingKillPending())
        {
            OldActor->Destroy();
        }

        TileToTerrainActorMap.Remove(Tile);
    }

    // Decide classe a spawnar
    TSubclassOf<ATerrainActor> ClassToSpawn = nullptr;

    switch (NewState)
    {
    case ETileState::Empty:
        ClassToSpawn = EmptyActorClass;
        break;
    case ETileState::Plowed:
        ClassToSpawn = PlowedActorClass;
        break;
    case ETileState::Planted:
        ClassToSpawn = PlantedActorClass;
        break;
    case ETileState::Grown:
        ClassToSpawn = GrownActorClass;
        break;
    default:
        break;
    }

    if (!ClassToSpawn) return;

    FVector WorldLocation = GetCenteredWorldLocationFromTile(Tile.X, Tile.Y);

    FActorSpawnParameters SpawnParams;
    ATerrainActor* NewActor = GetWorld()->SpawnActor<ATerrainActor>(
        ClassToSpawn,
        WorldLocation,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (NewActor)
    {
        NewActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        TileToTerrainActorMap.Add(Tile, NewActor);
    }
}
