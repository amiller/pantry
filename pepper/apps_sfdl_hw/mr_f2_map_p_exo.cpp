#include <apps_sfdl_hw/mr_f2_map_p_exo.h>
#include <apps_sfdl_gen/mr_f2_map_cons.h>
#include <common/sha1.h>
#include <storage/configurable_block_store.h>

#pragma pack(push)
#pragma pack(1)

#include <apps_sfdl/mr_f2_map.c>

//This file will NOT be overwritten by the code generator, if it already
//exists. make clean will also not remove this file.

mr_f2_mapProverExo::mr_f2_mapProverExo() { 
  baseline_minimal_input_size = sizeof(MapperIn);
  baseline_minimal_output_size = sizeof(MapperOut);
}

using namespace mr_f2_map_cons;

void mr_f2_mapProverExo::init_exo_inputs(
  const mpq_t* input_q, int num_inputs,
  char *folder_path, HashBlockStore *bs) {
  _bs = bs;

  MapperIn mapper_in;
  hash_t digest;

  for (int i=0; i<num_inputs; i++) {
    digest.bit[i] = mpz_get_ui(mpq_numref(input_q[i]));
  }
 
  import_exo_inputs_to_bs(bs, &mapper_in, sizeof(MapperIn), &digest); 
}

void mr_f2_mapProverExo::export_exo_inputs(
  const mpq_t* output_q, int num_outputs,
  char* folder_path, HashBlockStore *bs) {
  MapperChunkOut mapper_chunk_out;
  hash_t digest;

  //IMPORTANT: 0th element in output_q is return status!! 
  int k=1;
  for (int i=0; i<NUM_REDUCERS; i++) {
    for (int j=0; j<NUM_HASH_CHUNKS; j++) {
      digest.bit[j] = mpz_get_ui(mpq_numref(output_q[k]));
      k++;
    }
    export_exo_inputs_from_bs(bs, &mapper_chunk_out, sizeof(MapperChunkOut), &digest);
  }
}

void mr_f2_mapProverExo::run_shuffle_phase(char *folder_path) {
}

void mr_f2_mapProverExo::baseline_minimal(void* input, void* output){
  MapperIn* mapper_input = (MapperIn*)input;
  MapperOut* mapper_output = (MapperOut*)output;

  ::map(mapper_input, mapper_output);
}

void mr_f2_mapProverExo::baseline(const mpq_t* input_q, int num_inputs, 
      mpq_t* output_recomputed, int num_outputs) {
  MapperIn mapper_in;
  MapperOut mapper_out;

  hash_t digest;
  for (int i=0; i<num_inputs; i++) {
    digest.bit[i] = mpz_get_ui(mpq_numref(input_q[i]));
  }

  hashget2(_bs, &mapper_in, &digest);

  //computation
  baseline_minimal(&mapper_in, &mapper_out);
  // Fill code here to dump output to output_recomputed.
  //IMPORTANT: 0th element in output_q is return status!! 
  mpq_set_ui(output_recomputed[0], 0, 1);
  int k = 1;
  for (int i=0; i<NUM_REDUCERS; i++) {
    hashput(&digest, &(mapper_out.output[i]));

    for (int j=0; j<NUM_HASH_CHUNKS; j++) {
      mpq_set_ui(output_recomputed[k], digest.bit[j], 1); 
      k++;
    }
  }
}

//Refer to apps_sfdl_gen/mr_f2_map_cons.h for constants to use in this exogenous
//check.
bool mr_f2_mapProverExo::exogenous_check(const mpz_t* input, const mpq_t* input_q,
      int num_inputs, const mpz_t* output, const mpq_t* output_q, int num_outputs, mpz_t prime) {

  bool passed_test = true;
#ifdef ENABLE_EXOGENOUS_CHECKING
  mpq_t *output_recomputed;
  alloc_init_vec(&output_recomputed, num_outputs);
  baseline(input_q, num_inputs, output_recomputed, num_outputs);

  for(int i = 0; i < num_outputs; i++){
    if (mpq_equal(output_recomputed[i], output_q[i]) == 0){
      passed_test = false;
      break;
    }
  }
  clear_vec(num_outputs, output_recomputed);
#else
  gmp_printf("<Exogenous check disabled>\n");
#endif
  return passed_test;
};
#pragma pack(pop)
