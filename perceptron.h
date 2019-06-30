//16640

#include <stdio.h>
#include <string.h>
#include <math.h>

// Data for perceptron predictor

#define num_of_perceptrons 427
#define num_of_weights 23
#define p_SATUATELEN 8
#define hash_select(x) ((x * num_of_weights) % num_of_perceptrons)

int16_t history_register[num_of_weights]; //history register
int16_t p_W[num_of_perceptrons][num_of_weights + 1]; //nth perceptron -> weights, extra 1 for bias
int32_t theta = (int32_t)(1.93 * num_of_weights + 14);
uint8_t p_need_train = 0;
uint8_t recent_prediction = NOTTAKEN;



//======================Perceptron Predictor==================================

void shift_PERCEPTRON(int16_t* satuate, uint8_t same){ // satuate is xi, same is t*xi
  // weight ranges from - 2^(n-1) to 2^(n-1) - 1
  if(same){
    if(*satuate != ((1 << (p_SATUATELEN - 1)) - 1)){
      //printf("%d\n", *satuate);
      (*satuate)++;
    }
  }else{
    if(*satuate != -(1 << (p_SATUATELEN - 1 ) )){
      //printf("%d\n", *satuate);
      (*satuate)--;
    }
  }
}

void init_PERCEPTRON(){
  //printf("percep: PC_size: %d\theight: %d\tsatuate_len: %d\n", num_of_perceptrons, num_of_weights, p_SATUATELEN);
  memset(p_W, 0, sizeof(int16_t) * num_of_perceptrons * (num_of_weights + 1));
  memset(history_register, 0, sizeof(uint16_t) * num_of_weights);
}



uint8_t prediction_PERCEPTRON(uint32_t pc){
  uint32_t index = hash_select(pc);
  int16_t out = p_W[index][0]; //bias

  for(int i = 1 ; i <= num_of_weights ; i++){
    out += history_register[i-1] ? p_W[index][i] : -p_W[index][i];
  }

  recent_prediction = (out >= 0) ? TAKEN : NOTTAKEN;
  p_need_train = (out < theta && out > -theta) ? 1 : 0;

  return recent_prediction;
}

void update_PERCEPTRON(uint32_t pc, uint8_t outcome){

  uint32_t index = hash_select(pc);

  if((recent_prediction != outcome) || p_need_train){
    shift_PERCEPTRON(&(p_W[index][0]), outcome); // update bias
    for(int i = 1 ; i <= num_of_weights ; i++){ // since we regard bias as w0, index starts from 1
      uint8_t predict = history_register[i-1];
      shift_PERCEPTRON(&(p_W[index][i]), (outcome == predict)); // outcome = t, prediction = xi, when outcome agrees with prediction, xi++, else xi--.
    }

  }

  for(int i = num_of_weights - 1; i > 0 ; i--){
    history_register[i] = history_register[i-1];
  }
  history_register[0] = outcome;

}