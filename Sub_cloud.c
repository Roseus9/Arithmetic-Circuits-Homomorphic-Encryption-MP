#include <tfhe/tfhe.h>
#include <tfhe/tfhe_io.h>
#include <stdio.h>
#include <time.h>

void full_adder(LweSample *sum, const LweSample *x, const LweSample *y, const LweSample *c, const int32_t nb_bits, const TFheGateBootstrappingCloudKeySet *keyset)
{
	const LweParams *in_out_params = keyset->params->in_out_params;



	// carries
	/*LweSample *carry = new_LweSample_array(2, in_out_params);
	bootsSymEncrypt(carry, 0, keyset);*/ // first carry initialized to 0
	// temps
	LweSample *temp = new_LweSample_array(10, in_out_params);
	LweSample *tempcarry = new_LweSample_array(10, in_out_params);
	bootsCOPY(tempcarry, c, keyset);



	for (int32_t i = 0; i < nb_bits; ++i)
	{
		//sumi = xi XOR yi XOR carry(i-1)
		bootsXOR(temp, x + i, y + i, keyset); // temp = xi XOR yi
		bootsXOR(sum + i, temp, tempcarry, keyset);

		// carry = (xi AND yi) XOR (carry(i-1) AND (xi XOR yi))
		bootsNOT(temp + 3, x + i, keyset);
		bootsAND(temp + 1, temp + 3, y + i, keyset); // temp1 = xi AND yi
		bootsNOT(temp + 4, temp, keyset);
		bootsAND(temp + 2, tempcarry, temp + 4, keyset); // temp2 = carry AND temp
		bootsXOR(tempcarry + 1, temp + 1, temp + 2, keyset);
		bootsCOPY(tempcarry, tempcarry + 1, keyset);
	}



	//bootsCOPY(sum + nb_bits, tempcarry, keyset);
	//bootsCOPY(sum, tempcarry, keyset);
	//bootsCOPY(nb_bits, tempcarry, keyset);



	delete_LweSample_array(3, temp);
	delete_LweSample_array(2, tempcarry);
} 

int main() {
    
    printf("reading the key...\n");

    //reads the cloud key from file
    FILE* cloud_key = fopen("cloud.key","rb");
    TFheGateBootstrappingCloudKeySet* bk = new_tfheGateBootstrappingCloudKeySet_fromFile(cloud_key);
    fclose(cloud_key);
 
    //if necessary, the params are inside the key
    const TFheGateBootstrappingParameterSet* params = bk->params;

    printf("reading the input...\n");

    //read the 2x16 ciphertexts
    LweSample* ciphertext1 = new_gate_bootstrapping_ciphertext_array(16, params);
    LweSample* ciphertext2 = new_gate_bootstrapping_ciphertext_array(16, params);
	LweSample* ciphertext3 = new_gate_bootstrapping_ciphertext_array(16, params);

	

    //reads the 2x16 ciphertexts from the cloud file
    FILE* cloud_data = fopen("cloud.data","rb");
    for (int i=0; i<16; i++) 
        import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &ciphertext1[i], params);


    for (int i=0; i<16; i++) 
        import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &ciphertext2[i], params);



	for (int i = 0; i<16; i++)
		import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &ciphertext3[i], params);
	

    fclose(cloud_data);

    printf("doing the homomorphic computation...\n");
    
    //do some operations on the ciphertexts: here, we will compute the
    //minimum of the two
    LweSample* result = new_gate_bootstrapping_ciphertext_array(16, params);
    time_t start_time = clock();
    full_adder(result, ciphertext1, ciphertext2, ciphertext3, 16, bk);
    time_t end_time = clock();

    printf("......computation of the 16 binary + 32 mux gates took: %ld microsecs\n",end_time-start_time);
    
    printf("writing the answer to file...\n");
    
    //export the 32 ciphertexts to a file (for the cloud)
    FILE* answer_data = fopen("answer.data","wb");
    for (int i=0; i<16; i++) 
        export_gate_bootstrapping_ciphertext_toFile(answer_data, &result[i], params);
    fclose(answer_data);

    //clean up all pointers
    delete_gate_bootstrapping_ciphertext_array(16, result);
    delete_gate_bootstrapping_ciphertext_array(16, ciphertext2);
    delete_gate_bootstrapping_ciphertext_array(16, ciphertext1);
	delete_gate_bootstrapping_ciphertext_array(16, ciphertext3);
    delete_gate_bootstrapping_cloud_keyset(bk);

}
