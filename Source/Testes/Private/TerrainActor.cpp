// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainActor.h"

// Sets default values
ATerrainActor::ATerrainActor()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootComponent);

    UpdateMeshOffset(); // ⬅️ Aplica centralização inicial
}

// Called when the game starts or when spawned
void ATerrainActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATerrainActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATerrainActor::SetTileMesh(UStaticMesh* NewMesh)
{
    if (NewMesh && MeshComponent)
    {
        MeshComponent->SetStaticMesh(NewMesh);
    }
}

void ATerrainActor::SetTileSize(float InTileSize)
{
    TileSize = InTileSize;
    UpdateMeshOffset();
}

void ATerrainActor::UpdateMeshOffset()
{
    // ⬅️ Centraliza o mesh no tile
    MeshComponent->SetRelativeLocation(FVector(-TileSize / 2.f, -TileSize / 2.f, 0.f));
}

