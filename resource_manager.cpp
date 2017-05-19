#include "resource_manager.h"
#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <ftw.h>

int32_t ReadPaths(asset_diff* AssedDiffs, path* Paths, struct stat* Stats, int32_t* ElementCount,
                  const char* StartPath, const char* Extension);
namespace Resource
{
  void
  resource_hash_table::Set(rid RID, const void* Asset, const char* Path)
  {
    assert(0 < RID.Value && RID.Value <= TEXT_LINE_MAX_LENGTH);

    this->Assets[RID.Value - 1] = (void*)Asset;

    if(Path)
    {
      size_t PathLength = strlen(Path);
      assert(PathLength < TEXT_LINE_MAX_LENGTH);
      strcpy(this->Paths[RID.Value - 1].Name, Path);
    }
    else
    {
      memset(this->Paths[RID.Value - 1].Name, 0, TEXT_LINE_MAX_LENGTH * sizeof(char));
    }
  }

  bool
  resource_hash_table::Get(rid RID, void** Asset, char** Path)
  {
    assert(0 < RID.Value && RID.Value <= TEXT_LINE_MAX_LENGTH);

    if(Asset)
    {
      *Asset = this->Assets[RID.Value - 1];
    }

    if(Path)
    {
      *Path = this->Paths[RID.Value - 1].Name;
    }
    return true;
  }

  bool
  resource_hash_table::GetPathRID(rid* RID, const char* Path)
  {
    if(!Path)
    {
      assert(Path && "Queried path is NULL");
      return false;
    }
    for(int i = 0; i < RESOURCE_MANAGER_RESOURCE_CAPACITY; i++)
    {
      if(strcmp(Path, this->Paths[i].Name) == 0)
      {
        RID->Value = { i + 1 };
        return true;
      }
    }
    return false;
  }

  bool
  resource_hash_table::NewRID(rid* RID)
  {
    for(int i = 0; i < RESOURCE_MANAGER_RESOURCE_CAPACITY; i++)
    {
      if(this->Paths[i].Name[0] == '\0' && this->Assets[i] == NULL)
      {
        rid NewRID = { i + 1 };
        *RID       = NewRID;
        return true;
      }
    }
    assert(0 && "resource_manager error: unable to find new rid");
    return false;
  }

  bool
  resource_manager::LoadModel(rid RID)
  {
    Render::model* Model;
    char*          Path;
    if(this->Models.Get(RID, (void**)&Model, &Path))
    {
      if(Model)
      {
        assert(0 && "Reloading model");
      }
      else
      {
        debug_read_file_result AssetReadResult = ReadEntireFile(&this->ModelStack, Path);

        assert(AssetReadResult.Contents);
        if(AssetReadResult.ContentsSize <= 0)
        {
          return false;
        }
        Asset::asset_file_header* AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;

        UnpackAsset(AssetHeader);

        Model = (Render::model*)AssetHeader->Model;
        assert(Model);

        // Post load initialization
        for(int i = 0; i < Model->MeshCount; i++)
        {
          SetUpMesh(Model->Meshes[i]);
        }

        this->Models.Set(RID, Model, Path);
      }
      return true;
    }
    return false;
  }

  bool
  resource_manager::LoadTexture(rid RID)
  {
    char* Path;
    if(this->Textures.Get(RID, 0, &Path))
    {
      uint32_t TextureID = Texture::LoadTexture(Path);
      this->Textures.Set(RID, (void*)(uintptr_t)TextureID, Path);
      return true;
    }
    return false;
  }

  rid
  resource_manager::RegisterModel(const char* Path)
  {
    assert(Path);
    rid RID;
    assert(this->Models.NewRID(&RID));
    this->Models.Set(RID, NULL, Path);
    printf("registered model: rid %d, %s\n", RID.Value, Path);
    return RID;
  }

  bool
  resource_manager::GetModelPathRID(rid* RID, const char* Path)
  {
    return this->Models.GetPathRID(RID, Path);
  }

  bool
  resource_manager::GetTexturePathRID(rid* RID, const char* Path)
  {
    return this->Textures.GetPathRID(RID, Path);
  }

  rid
  resource_manager::RegisterTexture(const char* Path)
  {
    assert(Path);
    rid RID;
    assert(this->Textures.NewRID(&RID));
    this->Textures.Set(RID, NULL, Path);
    printf("registered texture: rid %d, %s\n", RID.Value, Path);
    return RID;
  }

  bool
  resource_manager::AsociateModel(rid RID, char* Path)
  {
    assert(Path);
    assert(0 < RID.Value && RID.Value <= RESOURCE_MANAGER_RESOURCE_CAPACITY);
    this->Models.Set(RID, NULL, Path);
    return true;
  }

  bool
  resource_manager::AsociateTexture(rid RID, char* Path)
  {
    assert(Path);
    assert(0 < RID.Value && RID.Value <= RESOURCE_MANAGER_RESOURCE_CAPACITY);
    this->Textures.Set(RID, NULL, Path);
    return true;
  }

  Render::model*
  resource_manager::GetModel(rid RID)
  {
    assert(0 < RID.Value && RID.Value <= RESOURCE_MANAGER_RESOURCE_CAPACITY);
    Render::model* Model;
    char*          Path;
    if(this->Models.Get(RID, (void**)&Model, &Path))
    {
      if(strcmp(Path, "") == 0)
      {
        assert(0 && "assert: No path associated with rid");
      }
      else if(Model)
      {
        return Model;
      }
      else
      {
        if(this->LoadModel(RID))
        {
          this->Models.Get(RID, (void**)&Model, &Path);
          printf("loaded model: rid %d, %s\n", RID.Value, Path);
          assert(Model);
          return Model;
        }
        else
        {
          return 0; // Should be default model here
        }
      }
    }
    else
    {
      assert(0 && "Invalid rid");
    }
    assert(0 && "Invalid codepath");
  }

  uint32_t
  resource_manager::GetTexture(rid RID)
  {
    assert(0 < RID.Value && RID.Value <= RESOURCE_MANAGER_RESOURCE_CAPACITY);
    uint32_t TextureID;
    char*    Path;
    if(this->Textures.Get(RID, (void**)&TextureID, &Path))
    {
      if(strcmp(Path, "") == 0)
      {
        printf("getting texture: rid %d\n", RID.Value);
        assert(0 && "assert: No path associated with rid");
      }
      else if(TextureID)
      {
        return TextureID;
      }
      else
      {
        if(this->LoadTexture(RID))
        {
          this->Textures.Get(RID, (void**)&TextureID, &Path);
          printf("loaded texture: rid %d, TexID %u, %s\n", RID.Value, TextureID, Path);
          assert(TextureID);
          return TextureID;
        }
        else
        {
          return -1; // Should be default model here
        }
      }
    }
    else
    {
      assert(0 && "assert: invalid rid");
    }
    assert(0 && "assert: invalid codepath");
  }

  void
  resource_manager::UpdateHardDriveDisplay()
  {
    // Update Models
    int DiffCount = ReadPaths(this->DiffedAssets, this->ModelPaths, this->ModelStats,
                              &this->ModelPathCount, "data", "model");
    if(DiffCount > 0)
    {
      printf("DIFF COUNT: %d\n", DiffCount);
      for(int i = 0; i < DiffCount; i++)
      {
        switch(this->DiffedAssets[i].Type)
        {
          case DIFF_Added:
            printf("Added: ");
            break;
          case DIFF_Modified:
            printf("Modified: ");
            break;
          case DIFF_Deleted:
            printf("Deleted: ");
            break;
          default:
            assert(0 && "assert: overflowed stat enum");
            break;
        }
        printf("%s\n", DiffedAssets[i].Path.Name);
      }
    }

    // Update Models
    DiffCount = ReadPaths(this->DiffedAssets, this->TexturePaths, this->TextureStats,
                          &this->TexturePathCount, "data/textures", NULL);
    if(DiffCount > 0)
    {
      printf("DIFF COUNT: %d\n", DiffCount);
      for(int i = 0; i < DiffCount; i++)
      {
        switch(this->DiffedAssets[i].Type)
        {
          case DIFF_Added:
            printf("Added: ");
            break;
          case DIFF_Modified:
            printf("Modified: ");
            break;
          case DIFF_Deleted:
            printf("Deleted: ");
            break;
          default:
            assert(0 && "assert: overflowed stat enum");
            break;
        }
        printf("%s\n", DiffedAssets[i].Path.Name);
      }
    }
  }
}

//-----------------------------------FILE QUERIES--------------------------------------

asset_diff*  g_DiffPaths;
path*        g_Paths;
struct stat* g_Stats;
int32_t*     g_ElementCount;
int32_t*     g_DiffCount;
const char*  g_Extension;
bool         g_WasTraversed[1000];

int32_t
GetPathIndex(path* PathArray, int32_t PathCount, const char* Path)
{
  int Index = -1;
  for(int i = 0; i < PathCount; i++)
  {
    if(strcmp(PathArray[i].Name, Path) == 0)
    {
      Index = i;
      break;
    }
  }
  return Index;
}

int32_t
CheckFile(const char* Path, const struct stat* Stat, int32_t Flag, struct FTW* FileName)
{
  if((Flag == FTW_D) && (Path[FileName->base] == '.'))
  {
    return FTW_SKIP_SUBTREE;
  }

  if(Flag == FTW_F)
  {
    if(Path[FileName->base] == '.')
    {
      return FTW_CONTINUE;
    }

    size_t PathLength = strlen(Path);
    if(PathLength > TEXT_LINE_MAX_LENGTH)
    {
      printf("%lu - %s\n", strlen(Path), Path);
    }
    assert(PathLength <= TEXT_LINE_MAX_LENGTH);

    if(g_Extension)
    {
      size_t ExtensionLength = strlen(g_Extension);
      if(0 < ExtensionLength)
      {
        if(PathLength <= ExtensionLength + 1)
        {
          return 0;
        }

        size_t ExtensionStartIndex = PathLength - ExtensionLength;
        if(!(Path[ExtensionStartIndex - 1] == '.' &&
             strcmp(&Path[ExtensionStartIndex], g_Extension) == 0))
        {
          return 0;
        }
      }
      else
      {
        assert(0 && "Extension length is less than or equal to zero! Did you mean to type NULL?");
        return 0;
      }
    }

    assert(*g_ElementCount < RESOURCE_MANAGER_RESOURCE_CAPACITY);
    assert(*g_DiffCount < 2 * RESOURCE_MANAGER_RESOURCE_CAPACITY);

    int PathIndex = GetPathIndex(g_Paths, *g_ElementCount, Path);
    if(PathIndex == -1)
    {
      strcpy(g_Paths[*g_ElementCount].Name, Path);
      g_Stats[*g_ElementCount]        = *Stat;
      g_WasTraversed[*g_ElementCount] = true;
      ++(*g_ElementCount);

      strcpy(g_DiffPaths[*g_DiffCount].Path.Name, Path);
      g_DiffPaths[*g_DiffCount].Type = DIFF_Added;
      ++(*g_DiffCount);
    }
    else
    {
      double TimeDiff = difftime(g_Stats[PathIndex].st_mtime, Stat->st_mtime);
      if(TimeDiff > 0)
      {
        printf("%f\n", TimeDiff);
        g_Stats[PathIndex] = *Stat;

        strcpy(g_DiffPaths[*g_DiffCount].Path.Name, Path);
        g_DiffPaths[*g_DiffCount].Type = DIFF_Modified;
        ++(*g_DiffCount);
      }
      g_WasTraversed[PathIndex] = true;
    }
  }
  return 0;
}

int32_t
ReadPaths(asset_diff* DiffPaths, path* Paths, struct stat* Stats, int32_t* ElementCount,
          const char* StartPath, const char* Extension)
{
  int32_t DiffCount = 0;

  g_DiffPaths    = DiffPaths;
  g_Paths        = Paths;
  g_Stats        = Stats;
  g_ElementCount = ElementCount;
  g_DiffCount    = &DiffCount;
  g_Extension    = Extension;
  for(int i = 0; i < sizeof(g_WasTraversed) / sizeof(g_WasTraversed[0]); i++)
  {
    g_WasTraversed[i] = false;
  }

  if(nftw(StartPath, CheckFile, 100, FTW_ACTIONRETVAL) != -1)
  {
    for(int i = 0; i < *ElementCount; i++)
    {
      if(!g_WasTraversed[i])
      {
        --(*ElementCount);
        Paths[i] = Paths[*ElementCount];
        Stats[i] = Stats[*ElementCount];

        DiffPaths[DiffCount].Path = Paths[i];
        DiffPaths[DiffCount].Type = DIFF_Deleted;
        ++DiffCount;
      }
    }
  }
  else
  {
    printf("Error occured when trying to access path %s\n", StartPath);
  }

  return DiffCount;
}
