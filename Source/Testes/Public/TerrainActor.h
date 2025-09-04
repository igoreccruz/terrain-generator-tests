// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerrainActor.generated.h"

UCLASS()
class TESTES_API ATerrainActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ATerrainActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terrain")
	UStaticMeshComponent* MeshComponent;

	UFUNCTION()
	void SetTileMesh(UStaticMesh* NewMesh);

	UFUNCTION()
	void SetTileSize(float InTileSize);  // ⬅️ Adicionado

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	float TileSize = 100.f;
	void UpdateMeshOffset();  // ⬅️ Responsável por centralizar o mesh

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
