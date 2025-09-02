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

    // Cria o leito do rio
    //FVector2D RiverStart(0, MapHeight * 50);     // ponto inicial (ajuste como quiser)
    //FVector2D RiverEnd(MapWidth * 100, MapHeight * 50); // ponto final
    //float RiverWidth = 300.0f;
    //float RiverDepth = 200.0f;

    //CarveRiver(RiverStart, RiverEnd, RiverWidth, RiverDepth);

    // Gera um caminho de rio curvo com 50 pontos
    FVector2D RiverStart(0, MapHeight * 50);
    FVector2D RiverEnd(MapWidth * 100, MapHeight * 50);
    float RiverWidth = 300.0f;
    float RiverDepth = 200.0f;

    MainRiverPath = GenerateCurvedRiverPath(50, RiverStart, RiverEnd, 300.0f, 3.0f);
    CarveCurvedRiver(MainRiverPath, RiverWidth, RiverDepth);

    // Armazena o caminho principal
    AllRiverPaths.Add(MainRiverPath);
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

}

void APerlinMapProceduralMeshGenerator::LevelTerrainAt(FVector WorldLocation, float Radius, float TargetHeight)
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
            // Aplicar falloff para suavizar a transição
            float Falloff = 1.0f - (Dist / Radius);

            // Interpola a altura atual até o valor desejado
            TerrainVertices[i].Z = FMath::Lerp(TerrainVertices[i].Z, TargetHeight, Falloff);
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
}

void APerlinMapProceduralMeshGenerator::CarveRiver(const FVector2D& Start, const FVector2D& End, float Width, float Depth)
{
    if (TerrainVertices.Num() == 0) return;

    for (int32 i = 0; i < TerrainVertices.Num(); ++i)
    {
        FVector& Vertex = TerrainVertices[i];

        // Calcula distância do vértice à linha do rio (convertendo para FVector)
        float Distance = FMath::PointDistToLine(
            FVector(Vertex.X, Vertex.Y, 0.0f),
            FVector(End.X - Start.X, End.Y - Start.Y, 0.0f),
            FVector(Start.X, Start.Y, 0.0f)
        );

        if (Distance <= Width)
        {
            float Falloff = 1.0f - (Distance / Width);
            Vertex.Z -= Depth * Falloff;

            // Adiciona água
            if (WaterISM)
            {
                FVector WaterLocation(Vertex.X, Vertex.Y, Vertex.Z + 1.0f);
                FTransform WaterTransform(FRotator::ZeroRotator, WaterLocation, FVector(1.0f));
                WaterISM->AddInstance(WaterTransform);
            }
        }
    }

    // Atualiza a mesh
    ProceduralMesh->UpdateMeshSection_LinearColor(
        0,
        TerrainVertices,
        TArray<FVector>(),     // Normals
        TArray<FVector2D>(),   // UVs
        TArray<FLinearColor>(),
        TArray<FProcMeshTangent>()
    );
}

TArray<FVector2D> APerlinMapProceduralMeshGenerator::GenerateCurvedRiverPath(int32 NumPoints, FVector2D Start, FVector2D End, float Amplitude, float Frequency)
{
    TArray<FVector2D> PathPoints;

    for (int32 i = 0; i <= NumPoints; ++i)
    {
        float T = (float)i / (float)NumPoints;
        FVector2D LinearPos = FMath::Lerp(Start, End, T);

        // Meandro senoidal
        float Offset = FMath::Sin(T * PI * Frequency) * Amplitude;

        // Direção perpendicular à linha (Start-End)
        FVector2D Direction = End - Start;
        Direction.Normalize();
        FVector2D Perp = FVector2D(-Direction.Y, Direction.X); // Perpendicular

        FVector2D CurvedPos = LinearPos + Perp * Offset;

        PathPoints.Add(CurvedPos);
    }

    return PathPoints;
}

void APerlinMapProceduralMeshGenerator::CarveCurvedRiver(const TArray<FVector2D>& RiverPath, float Width, float Depth)
{
    if (TerrainVertices.Num() == 0 || RiverPath.Num() == 0) return;

    for (int32 i = 0; i < TerrainVertices.Num(); ++i)
    {
        FVector& Vertex = TerrainVertices[i];
        FVector2D Vertex2D(Vertex.X, Vertex.Y);

        // Verifica a menor distância do ponto ao caminho
        float MinDistance = FLT_MAX;

        for (int32 j = 0; j < RiverPath.Num() - 1; ++j)
        {
            FVector2D SegmentStart = RiverPath[j];
            FVector2D SegmentEnd = RiverPath[j + 1];

            float Dist = FMath::PointDistToSegment(
                FVector(Vertex.X, Vertex.Y, 0.0f),
                FVector(SegmentStart.X, SegmentStart.Y, 0.0f),
                FVector(SegmentEnd.X, SegmentEnd.Y, 0.0f)
            );

            MinDistance = FMath::Min(MinDistance, Dist);
        }

        if (MinDistance <= Width)
        {
            float Falloff = 1.0f - (MinDistance / Width);
            Vertex.Z -= Depth * Falloff;

            // Instanciar água
            if (WaterISM)
            {
                FVector WaterLocation(Vertex.X, Vertex.Y, Vertex.Z + 1.0f);
                FTransform WaterTransform(FRotator::ZeroRotator, WaterLocation, FVector(1.0f));
                WaterISM->AddInstance(WaterTransform);
            }
        }
    }

    // Atualiza a mesh
    ProceduralMesh->UpdateMeshSection_LinearColor(
        0,
        TerrainVertices,
        TArray<FVector>(),     // Normals
        TArray<FVector2D>(),   // UVs
        TArray<FLinearColor>(),
        TArray<FProcMeshTangent>()
    );
}

//Gera o afluente a partir do ponto inicial do rio
//void APerlinMapProceduralMeshGenerator::AddTributaryAt(FVector StartLocation)
//{
//    if (MainRiverPath.Num() == 0) return;
//
//    FVector2D Start2D(StartLocation.X, StartLocation.Y);
//
//    // Escolhe um ponto aleatório no rio principal para conectar
//    int32 RiverIndex = FMath::RandRange(0, MainRiverPath.Num() - 1);
//    FVector2D End2D = MainRiverPath[RiverIndex];
//
//    // Gera caminho curvo do afluente até o rio
//    int32 NumPoints = 25;
//    float Amplitude = 150.0f;
//    float Frequency = 2.0f;
//
//    TArray<FVector2D> TributaryPath = GenerateCurvedRiverPath(NumPoints, Start2D, End2D, Amplitude, Frequency);
//
//    float TributaryWidth = 150.0f;
//    float TributaryDepth = 100.0f;
//
//    CarveCurvedRiver(TributaryPath, TributaryWidth, TributaryDepth);
//}

//Gera o afluente apartir do ponto mais proximo ao rio
//void APerlinMapProceduralMeshGenerator::AddTributaryAt(FVector StartLocation)
//{
//    if (MainRiverPath.Num() == 0 || TerrainVertices.Num() == 0) return;
//
//    FVector2D Start2D(StartLocation.X, StartLocation.Y);
//
//    // Encontra o ponto mais próximo no rio principal
//    float MinDistance = FLT_MAX;
//    FVector2D ClosestPoint = MainRiverPath[0];
//
//    for (const FVector2D& RiverPoint : MainRiverPath)
//    {
//        float Dist = FVector2D::Distance(Start2D, RiverPoint);
//        if (Dist < MinDistance)
//        {
//            MinDistance = Dist;
//            ClosestPoint = RiverPoint;
//        }
//    }
//
//    // Gera caminho curvado do ponto inicial até o rio
//    int32 NumPoints = 25;
//    float Amplitude = 150.0f; // menor que o rio principal
//    float Frequency = 2.0f;
//
//    TArray<FVector2D> TributaryPath = GenerateCurvedRiverPath(NumPoints, Start2D, ClosestPoint, Amplitude, Frequency);
//
//    float TributaryWidth = 150.0f;
//    float TributaryDepth = 100.0f;
//
//    CarveCurvedRiver(TributaryPath, TributaryWidth, TributaryDepth);
//}

void APerlinMapProceduralMeshGenerator::AddTributaryAt(FVector StartLocation)
{
    if (AllRiverPaths.Num() == 0 || TerrainVertices.Num() == 0) return;

    FVector2D Start2D(StartLocation.X, StartLocation.Y);

    // Encontra o ponto mais próximo em TODOS os caminhos existentes (rio principal e afluentes)
    float MinDistance = FLT_MAX;
    FVector2D ClosestPoint = FVector2D::ZeroVector;

    for (const TArray<FVector2D>& RiverPath : AllRiverPaths)
    {
        for (const FVector2D& Point : RiverPath)
        {
            float Dist = FVector2D::Distance(Start2D, Point);
            if (Dist < MinDistance)
            {
                MinDistance = Dist;
                ClosestPoint = Point;
            }
        }
    }

    // Gera caminho curvo do ponto até o mais próximo
    int32 NumPoints = 20;
    float Amplitude = 100.0f;
    float Frequency = 2.0f;

    TArray<FVector2D> TributaryPath = GenerateCurvedRiverPath(NumPoints, Start2D, ClosestPoint, Amplitude, Frequency);

    float TributaryWidth = 120.0f;
    float TributaryDepth = 80.0f;

    CarveCurvedRiver(TributaryPath, TributaryWidth, TributaryDepth);

    // Armazena o afluente recém-gerado
    AllRiverPaths.Add(TributaryPath);
}

void APerlinMapProceduralMeshGenerator::SimulateErosion(int32 NumIterations, float RainAmount, float ErosionStrength)
{
    if (TerrainVertices.Num() == 0 || MapWidth <= 0 || MapHeight <= 0)
        return;

    const int32 NumVertsX = MapWidth + 1;
    const int32 NumVertsY = MapHeight + 1;

    TArray<float> Water;
    Water.Init(0.0f, TerrainVertices.Num());

    TArray<float> Erosion;
    Erosion.Init(0.0f, TerrainVertices.Num());

    for (int32 Iter = 0; Iter < NumIterations; ++Iter)
    {
        // 1. Adiciona água (chuva)
        for (int32 i = 0; i < Water.Num(); ++i)
        {
            Water[i] += RainAmount;
        }

        // 2. Move água para os vizinhos mais baixos (simula escorrimento)
        for (int32 Y = 0; Y < NumVertsY; ++Y)
        {
            for (int32 X = 0; X < NumVertsX; ++X)
            {
                int32 Index = Y * NumVertsX + X;
                FVector Current = TerrainVertices[Index];
                float CurrentHeight = Current.Z + Water[Index];

                float LowestHeight = CurrentHeight;
                int32 LowestNeighbor = -1;

                // Verifica 4 vizinhos (N, S, E, W)
                TArray<FIntPoint> Neighbors = {
                    {X + 1, Y}, {X - 1, Y}, {X, Y + 1}, {X, Y - 1}
                };

                for (FIntPoint Offset : Neighbors)
                {
                    if (Offset.X < 0 || Offset.X >= NumVertsX || Offset.Y < 0 || Offset.Y >= NumVertsY)
                        continue;

                    int32 NeighborIndex = Offset.Y * NumVertsX + Offset.X;
                    float NeighborHeight = TerrainVertices[NeighborIndex].Z + Water[NeighborIndex];

                    if (NeighborHeight < LowestHeight)
                    {
                        LowestHeight = NeighborHeight;
                        LowestNeighbor = NeighborIndex;
                    }
                }

                // Se encontrou vizinho mais baixo, move parte da água
                if (LowestNeighbor != -1)
                {
                    float FlowAmount = (CurrentHeight - LowestHeight) * 0.5f;

                    FlowAmount = FMath::Clamp(FlowAmount, 0.0f, Water[Index]);

                    Water[Index] -= FlowAmount;
                    Water[LowestNeighbor] += FlowAmount;

                    // Marca erosão no local atual (água desgasta o solo)
                    Erosion[Index] += FlowAmount;
                }
            }
        }
    }

    // 3. Esculpe o terreno com base na erosão acumulada
    for (int32 i = 0; i < TerrainVertices.Num(); ++i)
    {
        TerrainVertices[i].Z -= Erosion[i] * ErosionStrength;
    }

    // 4. Atualiza a mesh
    ProceduralMesh->UpdateMeshSection_LinearColor(
        0,
        TerrainVertices,
        TArray<FVector>(),     // Normals
        TArray<FVector2D>(),   // UVs
        TArray<FLinearColor>(),
        TArray<FProcMeshTangent>()
    );
}

void APerlinMapProceduralMeshGenerator::SimulateErosionAt(FVector WorldLocation, float Radius, int32 NumIterations, float RainAmount, float ErosionStrength)
{
    if (TerrainVertices.Num() == 0 || MapWidth <= 0 || MapHeight <= 0)
        return;

    const int32 NumVertsX = MapWidth + 1;
    const int32 NumVertsY = MapHeight + 1;

    FVector LocalCenter = ProceduralMesh->GetComponentTransform().InverseTransformPosition(WorldLocation);

    TArray<float> Water;
    Water.Init(0.0f, TerrainVertices.Num());

    TArray<float> Erosion;
    Erosion.Init(0.0f, TerrainVertices.Num());

    // Pré-filtra os índices dentro do raio da erosão
    TArray<int32> AffectedIndices;
    for (int32 i = 0; i < TerrainVertices.Num(); ++i)
    {
        FVector2D Pos2D(TerrainVertices[i].X, TerrainVertices[i].Y);
        FVector2D Center2D(LocalCenter.X, LocalCenter.Y);
        float Dist = FVector2D::Distance(Pos2D, Center2D);
        if (Dist <= Radius)
        {
            AffectedIndices.Add(i);
        }
    }

    if (AffectedIndices.Num() == 0)
        return;

    for (int32 Iter = 0; Iter < NumIterations; ++Iter)
    {
        // 1. Chuva local
        for (int32 i : AffectedIndices)
        {
            Water[i] += RainAmount;
        }

        // 2. Espalhamento da água (escorrimento)
        for (int32 i : AffectedIndices)
        {
            int32 X = i % NumVertsX;
            int32 Y = i / NumVertsX;
            FVector Current = TerrainVertices[i];
            float CurrentHeight = Current.Z + Water[i];

            float LowestHeight = CurrentHeight;
            int32 LowestNeighbor = -1;

            TArray<FIntPoint> Neighbors = {
                {X + 1, Y}, {X - 1, Y}, {X, Y + 1}, {X, Y - 1}
            };

            for (FIntPoint Offset : Neighbors)
            {
                if (Offset.X < 0 || Offset.X >= NumVertsX || Offset.Y < 0 || Offset.Y >= NumVertsY)
                    continue;

                int32 NeighborIndex = Offset.Y * NumVertsX + Offset.X;
                float NeighborHeight = TerrainVertices[NeighborIndex].Z + Water[NeighborIndex];

                if (NeighborHeight < LowestHeight)
                {
                    LowestHeight = NeighborHeight;
                    LowestNeighbor = NeighborIndex;
                }
            }

            // Move água se possível
            if (LowestNeighbor != -1)
            {
                float FlowAmount = (CurrentHeight - LowestHeight) * 0.5f;
                FlowAmount = FMath::Clamp(FlowAmount, 0.0f, Water[i]);

                Water[i] -= FlowAmount;
                Water[LowestNeighbor] += FlowAmount;

                Erosion[i] += FlowAmount;

                // Se água encontrar ponto muito fundo (canal do rio), amplifica erosão
                if (TerrainVertices[LowestNeighbor].Z < Current.Z - 50.0f)
                {
                    Erosion[i] += FlowAmount * 1.5f; // Amplifica
                }
            }
        }
    }

    // 3. Aplica a erosão ao terreno
    for (int32 i : AffectedIndices)
    {
        TerrainVertices[i].Z -= Erosion[i] * ErosionStrength;
    }

    // 4. Atualiza a mesh
    ProceduralMesh->UpdateMeshSection_LinearColor(
        0,
        TerrainVertices,
        TArray<FVector>(),
        TArray<FVector2D>(),
        TArray<FLinearColor>(),
        TArray<FProcMeshTangent>()
    );
}
