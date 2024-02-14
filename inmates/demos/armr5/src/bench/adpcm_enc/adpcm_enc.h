#ifndef ADPC_ENC_H
#define ADPC_ENC_H

/*
time: 548
clock cycles: 2359931
*/

/*
WCET (8lvl) clock cycles: 84126240
*/

void adpcm_enc_init( void );
void adpcm_enc_main( void );
int adpcm_enc_return( void );

#endif /*ADPC_ENC_H*/