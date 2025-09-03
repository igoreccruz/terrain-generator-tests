// Fill out your copyright notice in the Description page of Project Settings.


#include "ACropActor.h"

// Sets default values
AACropActor::AACropActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

}

// Called when the game starts or when spawned
void AACropActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AACropActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AACropActor::SetCropMesh(UStaticMesh* Mesh)
{
    if (Mesh && MeshComponent)
    {
        MeshComponent->SetStaticMesh(Mesh);
    }
}

