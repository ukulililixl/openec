/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <arpa/inet.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "galois.h"
#include "jerasure.h"
#include "reed_sol.h"
#include "NativeJerasure.h"
#include "org_apache_hadoop.h"
#include "org_apache_hadoop_util_NativeJerasure.h"

void printMatrix(int* matrix, int row, int col) {
  int i=0, j=0;
  for (i=0; i<row; i++) {
    for (j=0; j<col; j++) {
      printf("%d ", matrix[i*col+j]);
    }
    printf("\n");
  }
}

void generate_matrix(int* mat, int rows, int cols, int w) {
  int k = cols;
  int n = rows;
  int m = n - k;

//  printf("k=%d, m=%d, n=%d\n",k,m,n);
  memset(mat, 0, rows * cols * sizeof(int));
  int i=0,j=0;
  for(i=0; i<k; i++) {
    mat[i*k+i] = 1;
  }

  for (i=0; i<m; i++) {
    int tmp = 1;
    for (j=0; j<k; j++) {
      mat[(i+k)*cols+j] = tmp;
      tmp = galois_single_multiply(tmp, i+1, w);
    }
  }
//  printMatrix(mat, rows, cols);
}

JNIEXPORT void JNICALL Java_org_apache_hadoop_util_NativeJerasure_nativeInit
  (JNIEnv *env, jclass clazz, jint stripeSize, jint paritySize) {
//  // xiaolu comment 20180831 start
//  matrix = reed_sol_vandermonde_coding_matrix(stripeSize, paritySize, WORD_SIZE);
//  // xiaolu comment 20180831 end

//  printf("NativeJerasure.nativeInit\n");
  // xiaolu add 20180831 start
  memset(matrix, 0, 32*32*sizeof(int));
  generate_matrix(matrix, stripeSize+paritySize, stripeSize, 8); 
  int i=0, j=0;
  for (i=0; i<paritySize; i++) {
    for (j=0; j<stripeSize; j++) {
      char tmpc = matrix[(stripeSize+i)*stripeSize + j];
      imatrix[i*stripeSize+j] = tmpc;
    }
  }
  // xiaolu add 20180831 end
}

void printArray(char** output, int num, int dataLength) {
  int i;
  int j;
  for (i = 0; i < num; i++) {
    printf("Row %d:", i);
    for (j = 0; j < dataLength; j++) {
      printf(" %d", output[i][j]);
    }
    printf("\n");
  }
}

void printTime(struct timespec* stime, const char* message) {
  struct timespec etime;
  clock_gettime(CLOCK_REALTIME, &etime);
  printf( "%s:%lfms\n", message, (etime.tv_nsec - stime->tv_nsec)/1e6);
}

JNIEXPORT void JNICALL Java_org_apache_hadoop_util_NativeJerasure_nativeEncodeBulk
  (JNIEnv *env, jclass clazz, jobjectArray inputBuffers, jobjectArray outputBuffers,
  jint stripeSize, jint paritySize, jint dataLength) {
//  printf("nativeEncodeBulk\n");

  char* data[stripeSize];
  char* coding[paritySize];
  int i;

  for (i = 0; i < stripeSize; i++) {
    jobject j_inputBuffer = (*env)->GetObjectArrayElement(env, inputBuffers, i);
    data[i] = (char*)(*env)->GetDirectBufferAddress(env, j_inputBuffer);
  }

  for (i = 0; i < paritySize; i++) {
    jobject j_outputBuffer = (*env)->GetObjectArrayElement(env, outputBuffers, i);
    coding[i] = (char*)(*env)->GetDirectBufferAddress(env, j_outputBuffer);
    memset(coding[i], 0, dataLength);
  }
//  // xiaolu comment 20180831 start
//  jerasure_matrix_encode(stripeSize, paritySize, WORD_SIZE, matrix, data, coding, dataLength);
//  // xiaolu comment 20180831 end
  unsigned char itable[32 * stripeSize * paritySize];
  ec_init_tables(stripeSize, paritySize, (unsigned char*)imatrix, itable);
  ec_encode_data(dataLength, stripeSize, paritySize, itable, (unsigned char**)data, (unsigned char**)coding);
}

// xiaolu add 20180831 start
JNIEXPORT void JNICALL Java_org_apache_hadoop_util_NativeJerasure_nativeDecodeBulk
  (JNIEnv *env, jclass clazz, jobjectArray inputBuffers, jobjectArray outputBuffers,
  jintArray erasedLocationsArray, jint dataLength, jint stripeSize,
  jint paritySize, jint numErasedLocations) {

  int i, eIdx, j, aIdx, k;

  // figure out erased locations and available locations
  int* rawErasures = (int *)malloc(sizeof(int)*(stripeSize+paritySize));
  for (i=0; i<stripeSize+paritySize; i++) rawErasures[i] = 0;
  jint* erasedLocations = (*env)->GetIntArrayElements(env, erasedLocationsArray, 0);
//  printf("erasedLocations: ");
  for (i=0; i<numErasedLocations; i++) {
    eIdx = erasedLocations[i];
//    printf("%d ", eIdx);
    if (eIdx < paritySize) eIdx += stripeSize;
    else eIdx -= paritySize;
    rawErasures[eIdx] = 1;
  }
//  printf("\n");
//  for (i=0; i<stripeSize+paritySize; i++) {
//    printf("rawErasures %d = %d\n", i, rawErasures[i]);
//  }
  int* erasures = (int *)malloc(numErasedLocations);
  int* availIdx = (int*)malloc(sizeof(int)*stripeSize);
  j=0;
  k=0;
  for (i=0; i<stripeSize+paritySize; i++) {
    if (rawErasures[i] == 1) {
      erasures[j++] = i;
    } else {
      if (k < stripeSize) {
        availIdx[k++] = i;
      }
    }
  }
//  printf("erasures: ");
//  for (i=0; i<numErasedLocations; i++) printf("%d ", erasures[i]);
//  printf("\n");
//  printf("availIdx: ");
//  for (i=0; i<stripeSize; i++) printf("%d ", availIdx[i]);
//  printf("\n");
  // select original matrix given availIdx
  int select_matrix[32*32];
  j=0;
  for (i=0; i<stripeSize; i++) {
    int sIdx = availIdx[i];
    // we copy the sIdx line in matrix to select_matrix
    memcpy(select_matrix + i * stripeSize, 
           matrix + sIdx * stripeSize,
           sizeof(int) * stripeSize);
  }
//  printf("select_matrix:\n");
//  printMatrix(select_matrix, stripeSize, stripeSize);
  int invert_matrix[32*32];
  jerasure_invert_matrix(select_matrix, invert_matrix, stripeSize, stripeSize);
//  printf("invert_matrix:\n");
//  printMatrix(invert_matrix, stripeSize, stripeSize);
  // get the lines of corrupt idx in matrix
  int select_vectors[32*32];
  for (i=0; i<numErasedLocations; i++) {
    int eIdx = erasures[i];
    memcpy(select_vectors + i * stripeSize,
           matrix + eIdx * stripeSize,
           sizeof(int) * stripeSize);
  }
//  printf("select_vectors:\n");
//  printMatrix(select_vectors, numErasedLocations, stripeSize);
  // get final matrix
  int* final_matrix = jerasure_matrix_multiply(
        select_vectors, invert_matrix, numErasedLocations, stripeSize, stripeSize, stripeSize, 8);
//  printf("final repair matrix:\n");
//  printMatrix(final_matrix, numErasedLocations, stripeSize);

  // first transfer the mat into char*
  char* dmatrix;
  dmatrix = (char*)calloc(numErasedLocations*stripeSize, sizeof(char));
  i=0, j=0;
  for (i=0; i<numErasedLocations*stripeSize; i++) {
      char tmpc = final_matrix[i];
      dmatrix[i] = tmpc;
  }
  // call isa-l library
  unsigned char itable[32 * 32 * 32];
  ec_init_tables(stripeSize, numErasedLocations, (unsigned char*)dmatrix, itable);

  // prepare data buffers
  char* availBuf[stripeSize];
  char* torecBuf[numErasedLocations];
  for (i=0; i<stripeSize; i++) {
    aIdx = availIdx[i];
    int tmpidx=0;
    if (aIdx < stripeSize) tmpidx = aIdx + paritySize;
    else tmpidx = aIdx - stripeSize;
    jobject j_inputBuffer = (*env)->GetObjectArrayElement(env, inputBuffers, tmpidx);
    availBuf[i] = (char*)(*env)->GetDirectBufferAddress(env, j_inputBuffer);
  }
  for (i=0; i<numErasedLocations; i++) {
    eIdx = erasures[i];
    int tmpidx=0;
    if (eIdx < stripeSize) tmpidx = eIdx + paritySize;
    else tmpidx = eIdx - stripeSize;
    jobject j_outputBuffer = (*env)->GetObjectArrayElement(env, outputBuffers, i);
    torecBuf[i] = (char*)(*env)->GetDirectBufferAddress(env, j_outputBuffer);
//    memset(torecBuf[i], 0, dataLength);
  }
  ec_encode_data(dataLength, stripeSize, numErasedLocations, itable, (unsigned char**)availBuf, (unsigned char**)torecBuf);
}
// xiaolu add 20180831 end

// // xiaolu comment 20180831 start
// JNIEXPORT void JNICALL Java_org_apache_hadoop_util_NativeJerasure_nativeDecodeBulk
//   (JNIEnv *env, jclass clazz, jobjectArray inputBuffers, jobjectArray outputBuffers,
//   jintArray erasedLocationsArray, jint dataLength, jint stripeSize,
//   jint paritySize, jint numErasedLocations) {
//   int* erasures = (int *)malloc(sizeof(int)*(stripeSize + paritySize));
//   char* data[stripeSize];
//   char* coding[paritySize];
//   int i;
//   char* outputBufs[numErasedLocations];
// 
//   for (i = 0; i < stripeSize + paritySize; i++) {
//     jobject j_inputBuffer = (*env)->GetObjectArrayElement(env, inputBuffers, i);
//     if (i < paritySize) {
//       coding[i] = (char*)(*env)->GetDirectBufferAddress(env, j_inputBuffer);
//     } else {
//       data[i - paritySize] = (char*)(*env)->GetDirectBufferAddress(env, j_inputBuffer);
//     }
//   }
// 
//   int index = 0;
//   jint* erasedLocations = (*env)->GetIntArrayElements(env, erasedLocationsArray, 0);
//   for (i = 0;i < numErasedLocations;i++) {
//     jobject j_outputBuffer = (*env)->GetObjectArrayElement(env, outputBuffers, i);
//     outputBufs[i] = (char*)(*env)->GetDirectBufferAddress(env, j_outputBuffer);
//     memset(outputBufs[i], 0, dataLength);
//     erasures[index] = erasedLocations[i];
//     index += 1;
//   }
//   erasures[index] = -1;
// 
//   for (i = 0;i < index;i++) {
//     if (erasures[i] < paritySize) {
//       erasures[i] += stripeSize;
//     } else {
//       erasures[i] -= paritySize;
//     }
//   }
// 
//   if (matrix == NULL) {
//     printf("Error setting matrix");
//   } else {
//     jerasure_matrix_decode(
//       stripeSize, paritySize, WORD_SIZE, matrix, 1, erasures, data, coding, dataLength);
//   }
// 
//   for (i = 0;i < numErasedLocations;i++) {
//     int eraseIndex = erasedLocations[i];
//     if (eraseIndex < paritySize) {
//       memcpy(outputBufs[i], coding[eraseIndex], dataLength);
//     } else {
//       memcpy(outputBufs[i], data[eraseIndex - paritySize], dataLength);
//     }
//   }
// }
// // xiaolu comment 20180831 end
