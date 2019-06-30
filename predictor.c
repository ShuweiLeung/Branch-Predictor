//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"
#include <string.h>
#include "perceptron.h"
#include "neural.h"

//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//

//------------------------------------//
//              GSHARE                //      
//------------------------------------//
uint8_t *gshareBHT;
uint32_t ghistory;

void init_GSHARE() {
  ghistory = 0;
  gshareBHT = malloc((1 << ghistoryBits) * sizeof(uint8_t));
  memset(gshareBHT, WN, (1 << ghistoryBits) * sizeof(uint8_t));
}

uint8_t prediction_GSHARE(uint32_t pc) {
  uint32_t GSHARE_BHT_index = (pc ^ ghistory) & ((1 << ghistoryBits) - 1);
  int GSHARE_state = gshareBHT[GSHARE_BHT_index];
  uint8_t prediction;
  if(GSHARE_state == ST || GSHARE_state == WT)
    prediction = TAKEN;
  else
    prediction = NOTTAKEN;
  return prediction;
}

void update_GSHARE(uint32_t pc, uint8_t outcome) {
  uint32_t GSHARE_BHT_index = (pc ^ ghistory) & ((1 << ghistoryBits) - 1);
  if(outcome == TAKEN) {
    if(gshareBHT[GSHARE_BHT_index] != ST)
      gshareBHT[GSHARE_BHT_index]++;
  }
  else {
    if(gshareBHT[GSHARE_BHT_index] != SN)
      gshareBHT[GSHARE_BHT_index]--;
  }

  ghistory = ghistory << 1 | outcome;
}


//------------------------------------//
//             TOURNAMENT             //      
//------------------------------------//
uint8_t *localPT;  //Local Prediction Table
uint32_t *localHT;   //Local History Table
uint8_t *choicePT; // Choice Prediction Table
uint8_t *globalPT;  // Global Prediction Table
uint32_t pathHistory;
uint8_t global_prediction;
uint8_t local_prediction;

void init_TOURNAMENT() {
  pathHistory = 0;
  localPT = malloc((1 << lhistoryBits) * sizeof(uint8_t)); //Local Prediction Table
  localHT = malloc((1 << pcIndexBits) * sizeof(uint32_t)); //Local History Table
  choicePT = malloc((1 << ghistoryBits) * sizeof(uint8_t)); //Choice Prediction Table
  globalPT = malloc((1 << ghistoryBits) * sizeof(uint8_t)); // Global Prediction Table
  memset(localPT, WN, (1 << lhistoryBits) * sizeof(uint8_t));
  memset(localHT, 0, (1 << pcIndexBits) * sizeof(uint32_t));
  memset(choicePT, 1, (1 << ghistoryBits) * sizeof(uint8_t));
  memset(globalPT, WN, (1 << ghistoryBits) * sizeof(uint8_t));
}

void prediction_TOURNAMENT_local(uint32_t pc) {
  uint32_t TOURNAMENT_localHT_index = pc & ((1 << pcIndexBits) - 1);
  uint32_t TOURNAMENT_localPT_index = localHT[TOURNAMENT_localHT_index];
  uint8_t TOURNAMENT_local_state = localPT[TOURNAMENT_localPT_index];
  uint8_t TOURNAMENT_local_prediction;
  if(TOURNAMENT_local_state == ST || TOURNAMENT_local_state == WT)
    TOURNAMENT_local_prediction = TAKEN;
  else
    TOURNAMENT_local_prediction = NOTTAKEN;

  local_prediction = TOURNAMENT_local_prediction;
}

void prediction_TOURNAMENT_global(uint32_t pc) {
  uint32_t TOURNAMENT_globalPT_index = pathHistory & ((1 << ghistoryBits) - 1);
  uint8_t TOURNAMENT_global_state = globalPT[TOURNAMENT_globalPT_index];
  uint8_t TOURNAMENT_global_prediction;
  if(TOURNAMENT_global_state == ST || TOURNAMENT_global_state == WT)
    TOURNAMENT_global_prediction = TAKEN;
  else
    TOURNAMENT_global_prediction = NOTTAKEN;
  global_prediction = TOURNAMENT_global_prediction;
}

uint8_t prediction_TOURNAMENT(uint32_t pc) {
  uint32_t TOURNAMENT_choicePT_index = pathHistory & ((1 << ghistoryBits) - 1);
  uint32_t predictor_choice = choicePT[TOURNAMENT_choicePT_index];
  prediction_TOURNAMENT_global(pc);
  prediction_TOURNAMENT_local(pc);

  if(predictor_choice <= 1) //choice = 00 or 01, choose global predictor
    return global_prediction;
  else
    return local_prediction; 
}

void update_TOURNAMENT(uint32_t pc, uint8_t outcome) {
  // update corresponding entry in choice table
  uint32_t TOURNAMENT_choicePT_index = pathHistory & ((1 << ghistoryBits) - 1);
  if(global_prediction != local_prediction) {
    if(global_prediction == outcome){
      if(choicePT[TOURNAMENT_choicePT_index] != 0)
        choicePT[TOURNAMENT_choicePT_index]--;
    }
    else {
      if(choicePT[TOURNAMENT_choicePT_index] != 3)
        choicePT[TOURNAMENT_choicePT_index]++;
    }
  }

  // update Global Prediction Table
  uint32_t TOURNAMENT_globalPT_index = pathHistory & ((1 << ghistoryBits) - 1);
  if(outcome == TAKEN) {
    if(globalPT[TOURNAMENT_globalPT_index] != ST)
      globalPT[TOURNAMENT_globalPT_index]++;
  }
  else {
    if(globalPT[TOURNAMENT_globalPT_index] != SN)
      globalPT[TOURNAMENT_globalPT_index]--;
  }

  // update path history
  pathHistory = pathHistory << 1 | outcome;
  pathHistory &= ((1 << ghistoryBits) - 1);

  // update Local Prediction Table
  uint32_t TOURNAMENT_localHT_index = pc & ((1 << pcIndexBits) - 1);
  uint32_t TOURNAMENT_localPT_index = localHT[TOURNAMENT_localHT_index];
  if(outcome == TAKEN) {
    if(localPT[TOURNAMENT_localPT_index] != ST)
      localPT[TOURNAMENT_localPT_index]++;
  }
  else {
    if(localPT[TOURNAMENT_localPT_index] != SN)
      localPT[TOURNAMENT_localPT_index]--;
  }

  // update Local History Table
  localHT[TOURNAMENT_localHT_index] = localHT[TOURNAMENT_localHT_index] << 1 | outcome;
  localHT[TOURNAMENT_localHT_index] &= ((1 << lhistoryBits) - 1);
}

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //

  switch (bpType) {
    case STATIC:
      break;
    case GSHARE:
      init_GSHARE();
      break;
    case TOURNAMENT:
      init_TOURNAMENT();
      break;
    case CUSTOM:
      init_PERCEPTRON();
      //neural_path_init();
    default:
      break;
  }

}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return prediction_GSHARE(pc);
    case TOURNAMENT:
      return prediction_TOURNAMENT(pc);
    case CUSTOM:
      return prediction_PERCEPTRON(pc);
      //return get_neural_prediction(pc);
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //

  switch (bpType) {
    case STATIC:
      break;
    case GSHARE:
      update_GSHARE(pc, outcome);
      break;
    case TOURNAMENT:
      update_TOURNAMENT(pc, outcome);
      break;
    case CUSTOM:
      //neural_train(pc, outcome);
      update_PERCEPTRON(pc, outcome);
    default:
      break;
  }

  return;
}
