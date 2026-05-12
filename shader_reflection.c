#define CINTERFACE 

#include <stdio.h>
#include "Windows.h"
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#pragma warning(disable : 4201)
#pragma warning(disable : 4115)

#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>

#include "arena.h"

#define NOT_FOUND -1
//I NEED MY EXPANDING ARENA IMPLEMENTATION MYYYYYYYYGOOOOOOOOOODDDDD



typedef struct{
    char file_name[MAX_PATH];
    ID3DBlob* vertex_shader;
    ID3DBlob* fragment_shader;
}ShaderResources;

typedef struct{
    ShaderResources* shader_resources;
    int num_shaders;
}ShaderBundle;

typedef enum{
    VERTEX_SHADER = 1,
    PIXEL_SHADER = 2
}SHADER_TYPE;
unsigned char backing_buffer[2048];
Arena resource_arena;
ShaderBundle bundle;

void d3d12_throwIfFailed(HRESULT hr){
    if(FAILED(hr)){
        assert(false);
        exit(EXIT_FAILURE);
    }
}
// once we find something, we can go back through the directory and check for it's counter part
//
int get_existing_shader_index(char* filename){
    for(int i = 0; i<bundle.num_shaders;i++){
        if(!strcmp(filename,bundle.shader_resources[i].file_name)){
            return i;
        }
    }
    return NOT_FOUND;
}
void set_bundle_data(char* file_handle,char* full_file_name,SHADER_TYPE shader_type){
    wchar_t full_path[MAX_PATH];
    wchar_t found_path[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, found_path);

    swprintf(full_path, MAX_PATH, L"%s\\%hs", found_path, full_file_name);
    //swprintf(full_path,MAX_PATH,L"C:\\Shader_Compiler\\%hs",full_file_name);
    char shader_name[MAX_PATH];
    int64_t total_chars = file_handle - full_file_name; 
    for(int i = 0;i<total_chars;i++){
        shader_name[i] = full_file_name[i];
    }
    shader_name[total_chars] = '\0';
    int shader_index = get_existing_shader_index(shader_name);
    if(shader_index == NOT_FOUND){
        assert(bundle.num_shaders<25);
        shader_index = bundle.num_shaders;
        strcpy(bundle.shader_resources[shader_index].file_name,shader_name);
        bundle.num_shaders++;
    }
    printf("%ws\n",full_path);
    if(shader_type == VERTEX_SHADER){
        d3d12_throwIfFailed(D3DReadFileToBlob(full_path,&bundle.shader_resources[shader_index].vertex_shader));
    }
    else if(shader_type == PIXEL_SHADER){
        d3d12_throwIfFailed(D3DReadFileToBlob(full_path,&bundle.shader_resources[shader_index].fragment_shader));
    }
    
}
void write_bindslot_def_to_file(FILE* fileptr,char* file_name,ID3D12ShaderReflection* reflect){
   
    D3D12_SHADER_DESC reflection_desc = {0};
    reflect->lpVtbl->GetDesc(reflect,&reflection_desc);
    
    for(unsigned int i = 0;i<reflection_desc.BoundResources;i++){
        D3D12_SHADER_INPUT_BIND_DESC bind_desc = {0};
        reflect->lpVtbl->GetResourceBindingDesc(reflect,i,&bind_desc);
        /*char preset = '\0';
        if(bind_desc.Type == D3D_SIT_CBUFFER){
            preset = 'B';
        }
        else if(bind_desc.Type == D3D_SIT_TEXTURE || bind_desc.Type == D3D_SIT_STRUCTURED){
            preset = 'T';
        }
        else if(bind_desc.Type == D3D_SIT_SAMPLER){
            preset = 'S';
        }
        else if(bind_desc.Type == D3D_SIT_UAV_RWTYPED || bind_desc.Type == D3D_SIT_UAV_RWSTRUCTURED || bind_desc.Type == D3D_SIT_UAV_APPEND_STRUCTURED || bind_desc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED || bind_desc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER){
            preset = 'U';   
        }*/
        fprintf(fileptr,"#define BINDSLOT_%s_%s %i\n",file_name,bind_desc.Name,bind_desc.BindPoint);
    }
    //the easiest way for us to do this is probably to just iterate through the resources again
}
void write_uniform_resources_to_struct(FILE* fileptr, ID3D12ShaderReflection* reflect){
    
    
    D3D12_SHADER_DESC reflection_desc = {0};
    reflect->lpVtbl->GetDesc(reflect,&reflection_desc);
    for(unsigned int i = 0;i<reflection_desc.BoundResources;i++){
        D3D12_SHADER_INPUT_BIND_DESC bind_desc = {0};
        reflect->lpVtbl->GetResourceBindingDesc(reflect,i,&bind_desc);
        if(bind_desc.Type == D3D_SIT_CBUFFER || bind_desc.Type == D3D_SIT_UAV_RWTYPED || 
            bind_desc.Type == D3D_SIT_UAV_RWSTRUCTURED || bind_desc.Type == D3D_SIT_UAV_APPEND_STRUCTURED || 
            bind_desc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED || bind_desc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER || bind_desc.Type == D3D_SIT_STRUCTURED){
            
            fprintf(fileptr,"\tslg_buffer %s;\n",bind_desc.Name);    
        }
        else if(bind_desc.Type == D3D_SIT_TEXTURE){
            fprintf(fileptr,"\tslg_texture %s;\n",bind_desc.Name);
        }
    }
}
void write_bind_function(FILE* fileptr,char* upper_filename, ID3D12ShaderReflection* reflect){
    D3D12_SHADER_DESC reflection_desc = {0};
    reflect->lpVtbl->GetDesc(reflect,&reflection_desc);
    for(unsigned int i = 0;i<reflection_desc.BoundResources;i++){
        D3D12_SHADER_INPUT_BIND_DESC bind_desc = {0};
        reflect->lpVtbl->GetResourceBindingDesc(reflect,i,&bind_desc);
        //for now we aren't doing uav's
        if(bind_desc.Type == D3D_SIT_CBUFFER ){
            fprintf(fileptr,"\tout_uniforms.cbv_buffer[BINDSLOT_%s_%s] = uniform_desc.%s;\n",upper_filename,bind_desc.Name,bind_desc.Name);    
        }
        if(bind_desc.Type == D3D_SIT_STRUCTURED){
            fprintf(fileptr,"\tout_uniforms.srv_buffer[BINDSLOT_%s_%s] = uniform_desc.%s;\n",upper_filename,bind_desc.Name,bind_desc.Name);
        }
        else if(bind_desc.Type == D3D_SIT_TEXTURE){
            fprintf(fileptr,"\tout_uniforms.srv_buffer[BINDSLOT_%s_%s] = uniform_desc.%s;\n",upper_filename,bind_desc.Name,bind_desc.Name); 
        }
        else if(bind_desc.Type == D3D_SIT_SAMPLER){
            fprintf(fileptr,"\tout_uniforms.samplers[BINDSLOT_%s_%s] = true;\n",upper_filename,bind_desc.Name);
        }
    }
}
int main(){
    //arena_init(&resource_arena,backing_buffer,sizeof(backing_buffer));
    bundle.num_shaders = 0;
    bundle.shader_resources = malloc(sizeof(ShaderResources) * 25);//reserve space for 25 shaders initially;
    assert(bundle.shader_resources!=NULL && "failed allocation");
    
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = FindFirstFileA(".\\*.*", &FindFileData);
    bool found_file = FindNextFile(hFind, &FindFileData);
    while(found_file){
        char* file_handle = strstr(FindFileData.cFileName,"_vs.cso");

        if(file_handle){
            set_bundle_data(file_handle,FindFileData.cFileName,VERTEX_SHADER);
            found_file = FindNextFile(hFind,&FindFileData);
            continue;
        }

        file_handle = strstr(FindFileData.cFileName,"_ps.cso");
        if(file_handle){ 
            set_bundle_data(file_handle,FindFileData.cFileName,PIXEL_SHADER);
            found_file = FindNextFile(hFind,&FindFileData);
            continue;
        }

        found_file = FindNextFile(hFind,&FindFileData);
    }

    
    FILE* fileptr;
    
    //before we go through all the files, we should make a central shader registry file
    fileptr = fopen("Shader_Registry.h","wb");


    fprintf(fileptr,"#ifndef SHADER_REGISTRY_H\n");
    fprintf(fileptr,"#define SHADER_REGISTRY_H\n\n");
        

    fprintf(fileptr,"\n#include \"slugs_graphics.h\"\n");

    //we need the shader name as well as the shader desc
    //go through each shader and add the include for the helper
    for(int i = 0;i<bundle.num_shaders;i++){
        char header_file_name[MAX_PATH];
        strcpy(header_file_name,bundle.shader_resources[i].file_name);
        strcat_s(header_file_name,sizeof(header_file_name),"_hlsl.h");
        
        fprintf(fileptr,"#include \"%s\"\n",header_file_name);
    }
    fprintf(fileptr,"\n");
    fprintf(fileptr, "typedef struct{\n");
    fprintf(fileptr, "\tconst char* hlsl_filename;\n");
    fprintf(fileptr, "\tslg_shader_desc shader_desc;\n");
    fprintf(fileptr, "\tslg_shader shd;\n");
    fprintf(fileptr, "}shader_registry_entry;\n");

    fprintf(fileptr,"shader_registry_entry shader_registry[%d];\n\n",bundle.num_shaders);
    fprintf(fileptr,"void init_shader_registry();\n");
    fprintf(fileptr,"slg_shader get_shader_from_registry(char* shader_name);\n");
    fprintf(fileptr,"#ifdef SHADER_REGISTRY_IMPLEMENTATION\n");
    fprintf(fileptr,"\nvoid init_shader_registry(){\n");
    fprintf(fileptr,"    int shader_registry_count = 4\n");
    for(int i = 0;i<bundle.num_shaders;i++){
        char uppr_file_name[MAX_PATH]; 
        memset(uppr_file_name,0,MAX_PATH);
        for(int c = 0;c<strlen(bundle.shader_resources[i].file_name);c++){
            uppr_file_name[c] = (char)toupper((unsigned char)bundle.shader_resources[i].file_name[c]);
        }
        char header_file_name[MAX_PATH];
        strcpy(header_file_name,bundle.shader_resources[i].file_name);
        strcat_s(header_file_name,sizeof(header_file_name),"_hlsl.h");
        fprintf(fileptr,"\tshader_registry[%d] = (shader_registry_entry)",i);
        fprintf(fileptr,"{\"%s\",%s_SHADER_DESC};\n",header_file_name,uppr_file_name);
    }
    fprintf(fileptr, "    for(int i = 0;i<shader_registry_count;i++){\n");
    fprintf(fileptr, "    \tshader_registry[i].shd = slg_make_shader(&shader_registry[i].shader_desc);\n");
    fprintf(fileptr, "    }\n");   
    fprintf(fileptr,"}\n");

    fprintf(fileptr,"slg_shader get_shader_from_registry(char* shader_name){\n");
    fprintf(fileptr,"    int shader_registry_count = 0;\n");
    fprintf(fileptr,"    for(int i = 0;i<shader_registry_count;i++){\n");
    fprintf(fileptr,"        if(!strcmp(shader_registry[i].hlsl_filename,shader_name)){\n");
    fprintf(fileptr,"            return shader_registry[i].shd;\n");
    fprintf(fileptr,"        }\n");
    fprintf(fileptr,"    }\n");
    fprintf(fileptr,"    return (slg_shader){0};\n");
    fprintf(fileptr,"}\n");
    fprintf(fileptr,"#endif //SHADER_REGISTRY_IMPLEMENTATION\n");
    fprintf(fileptr,"#endif //SHADER_REGISTRY_H\n");


    //now we need to go ahead an make the init function



    fclose(fileptr);


    //At this point we have all the files we need! We can make headers for each of the files with reflection data
    //we can iterate through all of our shaders
    for(int i = 0;i<bundle.num_shaders;i++){
        char header_file_name[MAX_PATH];
        
        strcpy(header_file_name,bundle.shader_resources[i].file_name);
        strcat_s(header_file_name,sizeof(header_file_name),"_hlsl.h");
        fileptr = fopen(header_file_name,"wb");
        if(!fileptr){
            assert(false);
        }

        ID3D12ShaderReflection* vert_reflection;
        ID3D12ShaderReflection* frag_reflection;
        ID3DBlob* vert_blob = bundle.shader_resources[i].vertex_shader;
        ID3DBlob* frag_blob = bundle.shader_resources[i].fragment_shader;
        d3d12_throwIfFailed(D3DReflect(vert_blob->lpVtbl->GetBufferPointer(vert_blob),
            vert_blob->lpVtbl->GetBufferSize(vert_blob),
            &IID_ID3D12ShaderReflection,
            (void**)&vert_reflection));
        d3d12_throwIfFailed(D3DReflect(frag_blob->lpVtbl->GetBufferPointer(frag_blob),
            frag_blob->lpVtbl->GetBufferSize(frag_blob),
            &IID_ID3D12ShaderReflection,
            (void**)&frag_reflection));
        
        fprintf(fileptr,"#ifndef %s_hlsl_h\n#define %s_hlsl_h\n",bundle.shader_resources[i].file_name,bundle.shader_resources[i].file_name);//print for the header guard

        //print lines for the slugs include statement
        fprintf(fileptr,"\n#if !defined(SLUGS_GRAPHICS_H)\n#error \"Please include slugs_graphics.h before including this file\"\n#endif\n");
        //fprintf(fileptr,"\n#ifndef BINDING_DEFITION\n#define BINDING_DEFINITION\n");//print beginning of guard for the shader header includes

        //printing the function pointer for the binding assignment
        /*fprintf(fileptr,"\ntypedef void (*UniformBinder)(void* uniforms, slg_bindings* bind);\n");
        fprintf(fileptr,"typedef struct{\n\tvoid* uniforms;\n\tUniformBinder bind_func;\n}Uniforms;\n");//Uniform Struct Definition
        */
        //fprintf(fileptr,"\n#endif\n\n");//ending guard for shader header includes
        
        char uppr_file_name[MAX_PATH]; 
        memset(uppr_file_name,0,MAX_PATH);
        for(int c = 0;c<strlen(bundle.shader_resources[i].file_name);c++){
            uppr_file_name[c] = (char)toupper((unsigned char)bundle.shader_resources[i].file_name[c]);
        }

        //writing out the filename definitions
        fprintf(fileptr, "#define HLSL_SHADER_SOURCE_%s \"%s.hlsl\"\n",uppr_file_name,bundle.shader_resources[i].file_name);
        if(bundle.shader_resources[i].vertex_shader != NULL){
            fprintf(fileptr,"#define VERTEX_SHADER_SOURCE_%s \"%s_vs.cso\" \n",uppr_file_name,bundle.shader_resources[i].file_name);
        }
        if(bundle.shader_resources[i].fragment_shader != NULL){
            fprintf(fileptr,"#define FRAGMENT_SHADER_SOURCE_%s \"%s_ps.cso\" \n",uppr_file_name,bundle.shader_resources[i].file_name);
        }


        write_bindslot_def_to_file(fileptr,uppr_file_name,vert_reflection);
        write_bindslot_def_to_file(fileptr,uppr_file_name,frag_reflection);

        fprintf(
            fileptr,
            "\n#define %s_SHADER_DESC (slg_shader_desc){\\\n"
            ".filename = \"%s.hlsl\",\\\n"
            ".vert_shader_target = \"vs_5_0\",\\\n"
            ".vert_shader_name = \"%s_vs.cso\",\\\n"
            ".frag_shader_target = \"ps_5_0\",\\\n"
            ".frag_shader_name = \"%s_ps.cso\"\\\n"
            "}\n",
            uppr_file_name,
            bundle.shader_resources[i].file_name,
            bundle.shader_resources[i].file_name,
            bundle.shader_resources[i].file_name
        );
        //These should just be #define with indexes and that's it I think        
        fprintf(fileptr,"\ntypedef struct %s_HLSL_UNIFORMS{\n",uppr_file_name);
        
        write_uniform_resources_to_struct(fileptr,vert_reflection);
        write_uniform_resources_to_struct(fileptr,frag_reflection);
        fprintf(fileptr,"}%s_HLSL_UNIFORMS;\n",uppr_file_name);

        //at this point we need to go through again to make the function
        fprintf(fileptr,"\nslg_uniforms %s_HLSL_MAKE_UNIFORMS(%s_HLSL_UNIFORMS uniform_desc){\n",uppr_file_name,uppr_file_name);
        fprintf(fileptr,"\tslg_uniforms out_uniforms = {0};\n");
        write_bind_function(fileptr,uppr_file_name,vert_reflection);
        write_bind_function(fileptr,uppr_file_name,frag_reflection);
        fprintf(fileptr,"\treturn out_uniforms;\n");
        fprintf(fileptr,"}\n");

        //last step I have to setup the macro that makes the generic uniform
        
        //fprintf(fileptr,"#define MAKE_%s_UNIFORMS(%s_UNIFORMS_STRUCT)\\\n\t(Uniforms){(void*)&%s_UNIFORMS_STRUCT,&set_%s_uniforms}\n",uppr_file_name,uppr_file_name,uppr_file_name,bundle.shader_resources[i].file_name);

        //final endif
        fprintf(fileptr,"#endif");//endif for the header guard
        //we want the file to have the proper header guards for sure
        

        //if(bundle.shader_resources[i].vertex_shader){
        //    
        //}
        

    }
    //fileptr = fopen(,"w");
}