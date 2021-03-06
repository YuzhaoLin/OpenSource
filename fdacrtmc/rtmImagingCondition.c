#include<stdlib.h>
#include<math.h>
#include<string.h>
#include"fdacrtmc.h"

//int MigDirDecompAcoustic4(modPar *mod, decompPar *decomp, wavPar *wav, fftPlansPar *fftPlans);
int extractMigrationSnapshots(modPar *mod, wavPar *wav, migPar *mig, decompPar *decomp);


float* getFArrPointer();
size_t decompresSnapshot(void* in);


int rtmImagingCondition(modPar *mod, wavPar *wav, migPar *mig, decompPar *decomp, fftPlansPar *fftPlans){
	fftw_complex *S, *R, *tmp;
	float pxf, pzf, pxb, pzb;
	float *tap, *farr;
	size_t ix, iz, ix1, iz1;

	switch(mig->mode){
		case 1: /* Conventional zero-lag pressure cross-correlation */
			mig->it--;

			// Decompress Source Wavefield?
			if(mig->compress){
				decompresSnapshot((void*)mig->wav[mig->it].tzz);
				farr=getFArrPointer();
			}else farr=mig->wav[mig->it].tzz;

			//Apply Imaging Condition
			for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
				for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
					mig->image[ix*mig->nz+iz]+=mig->dt*(wav->tzz[ix1*mod->naz+iz1]*farr[ix*mig->nz+iz]);
				}
			}
			free(mig->wav[mig->it].tzz);
			break;
		case 2: /* Poynting decomposition zero-lag pressure cross-correlation */
			mig->it--;
			switch(mig->orient){
				case 0: //Cross-correlate wavefields travelling in (un)like directions
					if(mig->backscatter){
						for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
							for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
								/* Source Wavefield Pointing Vector */
								pxf=mig->wav[mig->it].tzz[ix*mig->nz+iz]*mig->wav[mig->it].vx[ix*mig->nz+iz];
								pzf=mig->wav[mig->it].tzz[ix*mig->nz+iz]*mig->wav[mig->it].vz[ix*mig->nz+iz];
								/* Receiver Wavefield Pointing Vector */
								pxb=wav->tzz[ix1*mod->naz+iz1]*(wav->vx[ix1*mod->naz+iz1]+wav->vx[(ix1+1)*mod->naz+iz1])/2.0;
								pzb=wav->tzz[ix1*mod->naz+iz1]*(wav->vz[ix1*mod->naz+iz1]+wav->vz[ix1*mod->naz+iz1+1])/2.0;
								/* Image only where wavefields are travelling in the same direction */
								if(pxf*pxb>0&&pzf*pzb>0) mig->image[ix*mig->nz+iz]+=mig->dt*(wav->tzz[ix1*mod->naz+iz1]*mig->wav[mig->it].tzz[ix*mig->nz+iz]);
							}
						}
					}else{
						for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
							for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
								/* Source Wavefield Pointing Vector */
								pxf=mig->wav[mig->it].tzz[ix*mig->nz+iz]*mig->wav[mig->it].vx[ix*mig->nz+iz];
								pzf=mig->wav[mig->it].tzz[ix*mig->nz+iz]*mig->wav[mig->it].vz[ix*mig->nz+iz];
								/* Receiver Wavefield Pointing Vector */
								pxb=wav->tzz[ix1*mod->naz+iz1]*(wav->vx[ix1*mod->naz+iz1]+wav->vx[(ix1+1)*mod->naz+iz1])/2.0;
								pzb=wav->tzz[ix1*mod->naz+iz1]*(wav->vz[ix1*mod->naz+iz1]+wav->vz[ix1*mod->naz+iz1+1])/2.0;
								/* Image only where wavefields are not travelling in the same direction */
								if(pxf*pxb<0||pzf*pzb<0) mig->image[ix*mig->nz+iz]+=mig->dt*(wav->tzz[ix1*mod->naz+iz1]*mig->wav[mig->it].tzz[ix*mig->nz+iz]);
							}
						}
					}
					free(mig->wav[mig->it].vx);
					free(mig->wav[mig->it].vz);
					break;
				case 1: // Up-Down Imaging
					for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
						for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
							/* Source Wavefield Pointing Vector */
							pzf=mig->wav[mig->it].tzz[ix*mig->nz+iz]*mig->wav[mig->it].vz[ix*mig->nz+iz];
							/* Receiver Wavefield Pointing Vector */
							pzb=wav->tzz[ix1*mod->naz+iz1]*(wav->vz[ix1*mod->naz+iz1]+wav->vz[ix1*mod->naz+iz1+1])/2.0;
							/* Image only where wavefields are not travelling in the same direction */
							if(pzf*pzb>0) mig->image[ix*mig->nz+iz]+=mig->dt*(wav->tzz[ix1*mod->naz+iz1]*mig->wav[mig->it].tzz[ix*mig->nz+iz]);
						}
					}
					free(mig->wav[mig->it].vz);
					break;
				case 2: // Left-Right Imaging
					for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
						for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
							/* Source Wavefield Pointing Vector */
							pxf=mig->wav[mig->it].tzz[ix*mig->nz+iz]*mig->wav[mig->it].vx[ix*mig->nz+iz];
							/* Receiver Wavefield Pointing Vector */
							pxb=wav->tzz[ix1*mod->naz+iz1]*(wav->vx[ix1*mod->naz+iz1]+wav->vx[(ix1+1)*mod->naz+iz1])/2.0;
							/* Image only where wavefields are not travelling in the same direction */
							if(pxf*pxb<0) mig->image[ix*mig->nz+iz]+=mig->dt*(wav->tzz[ix1*mod->naz+iz1]*mig->wav[mig->it].tzz[ix*mig->nz+iz]);
						}
					}
					free(mig->wav[mig->it].vx);
					break;
				case 3: //Normal Imaging
					//Not yet implemented
					break;
				case 4: //Specific Direction Combination 
				/* Like pp (Down Source - Down Receiver) [Assuming + denotes down]
				        pm (Down Source - Up   Receiver) [Assuming + denotes down]
				        mp (Up   Source - Down Receiver) [Assuming + denotes down]
				        mm (Up   Source - Up   Receiver) [Assuming + denotes down] */
				//Not yet implemented
				break;
			}
			free(mig->wav[mig->it].tzz);
			break;
		case 3: /* Wavefield decomposition zero-lag pressure cross-correlation */
			mig->it--;break;
			/* Directionally Decompose Wavefield */
//			MigDirDecompAcoustic4(mod,decomp,wav,fftPlans);
			/* Apply Imaging Condition */
			switch(mig->orient){
				case 1: //Up-Down Imaging
					if(mig->backscatter){ //Image Back-Scattered Fields?
						// Cross-Correlate Wavefields Travelling In Same Directions
						for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
							for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
								mig->image[ix*mig->nz+iz]+=mig->dt*(wav->pd[ix1*mod->naz+iz1]*mig->wav[mig->it].pu[ix*mig->nz+iz]+
								                                    wav->pu[ix1*mod->naz+iz1]*mig->wav[mig->it].pd[ix*mig->nz+iz]);
							}
						}
					}else{
						// Cross-Correlate Wavefields Travelling In Opposite Directions
						for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
							for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
								mig->image[ix*mig->nz+iz]+=mig->dt*(wav->pd[ix1*mod->naz+iz1]*mig->wav[mig->it].pd[ix*mig->nz+iz]+
								                                    wav->pu[ix1*mod->naz+iz1]*mig->wav[mig->it].pu[ix*mig->nz+iz]);
							}
						}
					}
					free(mig->wav[mig->it].pu);
					free(mig->wav[mig->it].pd);
					break;
				case 2: //Left-Right Imaging
					if(mig->backscatter){ //Image Back-Scattered Fields?
						// Cross-Correlate Wavefields Travelling In Same Directions
						for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
							for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
								mig->image[ix*mig->nz+iz]+=mig->dt*(wav->pl[ix1*mod->naz+iz1]*mig->wav[mig->it].pl[ix*mig->nz+iz]+
								                                    wav->pr[ix1*mod->naz+iz1]*mig->wav[mig->it].pr[ix*mig->nz+iz]);
							}
						}
					}else{
						// Cross-Correlate Wavefields Travelling In Opposite Directions
						for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
							for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
								mig->image[ix*mig->nz+iz]+=mig->dt*(wav->pl[ix1*mod->naz+iz1]*mig->wav[mig->it].pr[ix*mig->nz+iz]+
								                                    wav->pr[ix1*mod->naz+iz1]*mig->wav[mig->it].pl[ix*mig->nz+iz]);
							}
						}
					}
					free(mig->wav[mig->it].pl);
					free(mig->wav[mig->it].pr);
					break;
				case 3: //Normal Imaging
					if(mig->backscatter){ //Image Back-Scattered Fields?
						// Cross-Correlate Wavefields Travelling In Same Directions
						for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
							for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
								mig->image[ix*mig->nz+iz]+=mig->dt*(wav->pn[ix1*mod->naz+iz1]*mig->wav[mig->it].pn[ix*mig->nz+iz]+
								                                   (wav->tzz[ix1*mod->naz+iz1]-wav->pn[ix1*mod->naz+iz1])*(mig->wav[mig->it].tzz[ix*mig->nz+iz]-mig->wav[mig->it].pn[ix*mig->nz+iz]));
							}
						}
					}else{
						// Cross-Correlate Wavefields Travelling In Opposite Directions
						for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
							for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
								mig->image[ix*mig->nz+iz]+=mig->dt*(wav->pn[ix1*mod->naz+iz1]*(mig->wav[mig->it].tzz[ix*mig->nz+iz]-mig->wav[mig->it].pn[ix*mig->nz+iz])+
								                                   (wav->tzz[ix1*mod->naz+iz1]-wav->pn[ix1*mod->naz+iz1])*mig->wav[mig->it].pn[ix*mig->nz+iz]);
							}
						}
					}
					free(mig->wav[mig->it].tzz);
					free(mig->wav[mig->it].pn);
					break;
			case 9991: //Testing Mode [Down (+) - Up (-)]
				//We multiply with mig->dt at the end, this saves doing it every loop iteration
				if(mig->pp){ //Cross-correlate down-going source wavefield with down-going receiver wavefield
					for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
						for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
							mig->image[ix*mig->nz+iz]+=mig->wav[mig->it].pd[ix*mig->nz+iz]*wav->pd[ix1*mod->naz+iz1];
						}
					}
				}
				if(mig->pm){ //Cross-correlate down-going source wavefield with up-going receiver wavefield
					for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
						for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
							mig->image[ix*mig->nz+iz]+=mig->wav[mig->it].pd[ix*mig->nz+iz]*wav->pu[ix1*mod->naz+iz1];
						}
					}
				}
				if(mig->mp){ //Cross-correlate up-going source wavefield with down-going receiver wavefield
					for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
						for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
							mig->image[ix*mig->nz+iz]+=mig->wav[mig->it].pu[ix*mig->nz+iz]*wav->pd[ix1*mod->naz+iz1];
						}
					}
				}
				if(mig->mm){ //Cross-correlate up-going source wavefield with down-going receiver wavefield
					for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
						for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
							mig->image[ix*mig->nz+iz]+=mig->wav[mig->it].pu[ix*mig->nz+iz]*wav->pu[ix1*mod->naz+iz1];
						}
					}
				}
				free(mig->wav[mig->it].pu);
				free(mig->wav[mig->it].pd);
				break;
			case 9992: //Testing Mode [Right (+) - Left (-)]
				// TODO: Implement
				break;
			case 9993: //Testing Mode [Normal (+) - Opposite (-)]
				// TODO: Implement
				break;
			}
			break;
		case 4: /* Plane-wave wavefield decomposition zero-lag pressure cross-correlation */
			// The imaging is not done now but will be done at the end of the job.
			// We now store the snapshots of the receiver wavefield.
			extractMigrationSnapshots(mod,wav,mig,decomp);
			break;
		case 5: /* Hilbert Transform Imaging Condition */
			mig->it--;

			/* Decompress Source Wavefield? */
			if(mig->compress){
				decompresSnapshot((void*)mig->wav[mig->it].tzz);
				farr=getFArrPointer();
			}else farr=mig->wav[mig->it].tzz;

			/* Compute Taper */
			if(mig->tap>1){
				tap=(float*)malloc((mig->tap-1)*sizeof(float));
				*tap=0;
				for(ix=1;ix<mig->tap-1;ix++){
					tap[ix]=sin(1.5707963267948966192313216916398*(((float)(ix))/((float)(mig->tap))));
					tap[ix]*=tap[ix];
				}
			}

			/* Apply Imaging Condition */
			switch(mig->orient){
				case 1: /* Up-Down Imaging*/
					/* Allocate Arrays */
					S  =(fftw_complex*)fftw_malloc(mig->nz*sizeof(fftw_complex));
					R  =(fftw_complex*)fftw_malloc(mig->nz*sizeof(fftw_complex));
					tmp=(fftw_complex*)fftw_malloc(mig->nz*sizeof(fftw_complex));
					/* Loop over horizontal coordinates */
					for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx;ix++,ix1+=mig->skipdx){
						// Hilbert Transform Source Wavefield
						if(mig->tap>1){ //Apply Taper?
							for(iz=0;iz<mig->tap-1;iz++)    S[iz]=(fftw_complex)(tap[iz]          *farr[ix*mig->nz+iz]);
							for(;iz<mig->nz-mig->tap+1;iz++)S[iz]=(fftw_complex)                   farr[ix*mig->nz+iz];
							for(;iz<mig->nz;iz++)           S[iz]=(fftw_complex)(tap[mig->nz-1-iz]*farr[ix*mig->nz+iz]);
						}else for(iz=0;iz<mig->nz;iz++)         S[iz]=(fftw_complex)                   farr[ix*mig->nz+iz];
						fftw_execute_dft(fftPlans->fft_1d_c2c_z,S,tmp); //Transform to Wavenumber Domain
						memset(&tmp[mig->nz/2+1],0,(mig->nz-mig->nz/2-1)*sizeof(fftw_complex)); //Mute Negative Wavenumbers
						fftw_execute_dft(fftPlans->ifft_1d_c2c_Kz,tmp,S); //Transform to Space Domain
						// Hilbert Transform Receiver Wavefield
						if(mig->tap>1){ //Apply Taper?
							for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->tap-1;iz++,iz1+=mig->skipdz)R[iz]=(fftw_complex)(tap[iz]          *wav->tzz[ix1*mod->naz+iz1]);
							for(;iz<mig->nz-mig->tap+1;iz++,iz1+=mig->skipdz)                  R[iz]=(fftw_complex)                   wav->tzz[ix1*mod->naz+iz1];
							for(;iz<mig->nz;iz++,iz1+=mig->skipdz)                             R[iz]=(fftw_complex)(tap[mig->nz-1-iz]*wav->tzz[ix1*mod->naz+iz1]);
						}else for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz)     R[iz]=(fftw_complex)                   wav->tzz[ix1*mod->naz+iz1];
						fftw_execute_dft(fftPlans->fft_1d_c2c_z,R,tmp); //Transform to Wavenumber Domain
						memset(&tmp[mig->nz/2+1],0,(mig->nz-mig->nz/2-1)*sizeof(fftw_complex)); //Mute Negative Wavenumbers
						fftw_execute_dft(fftPlans->ifft_1d_c2c_Kz,tmp,R); //Transform to Space Domain
						// Apply Imaging Condition
						for(iz=0;iz<mig->nz;iz++)mig->image[ix*mig->nz+iz]+=(float)creal(S[iz]*R[iz]); //TODO: Should you not be skipping values here for mig->dz?
						//Note: We multiply by 2*dt/nz^2 when writing to disk.
					}
					if(mig->tap>1) free(tap);
					break;
				case 2: /* Left-Right Imaging*/
					// Allocate Arrays
					S  =(fftw_complex*)fftw_malloc(mig->nx*sizeof(fftw_complex));
					R  =(fftw_complex*)fftw_malloc(mig->nx*sizeof(fftw_complex));
					tmp=(fftw_complex*)fftw_malloc(mig->nx*sizeof(fftw_complex));
					// Loop over horizontal coordinates
					for(iz=0,iz1=mod->ioPz+mig->z1;iz<mig->nz;iz++,iz1+=mig->skipdz){
						// Hilbert Transform Source Wavefield
						if(mig->tap>1){ //Apply Taper?
							for(ix=0;ix<mig->tap-1        ;ix++)S[ix]=(fftw_complex)(farr[ix*mig->nz+iz]*tap[ix]          );
							for(    ;ix<mig->nx-mig->tap+1;ix++)S[ix]=(fftw_complex) farr[ix*mig->nz+iz]                   ;
							for(    ;ix<mig->nx           ;ix++)S[ix]=(fftw_complex)(farr[ix*mig->nz+iz]*tap[mig->nx-1-ix]);
						}else	for(ix=0;ix<mig->nx           ;ix++)S[ix]=(fftw_complex)mig->wav[mig->it].tzz[ix*mig->nz+iz]   ;
						fftw_execute_dft(fftPlans->fft_1d_c2c_x,S,tmp); //Transform to Wavenumber Domain
						memset(&tmp[mig->nx/2+1],0,(mig->nx-mig->nx/2-1)*sizeof(fftw_complex)); //Mute Negative Wavenumbers
						fftw_execute_dft(fftPlans->ifft_1d_c2c_Kx,tmp,S); //Transform to Space Domain
						// Hilbert Transform Receiver Wavefield
						if(mig->tap>1){  //Apply Taper?
							for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->tap-1        ;ix++,ix1+=mig->skipdx)R[ix]=(fftw_complex)(wav->tzz[ix1*mod->naz+iz1]*tap[ix]          );
							for(                          ;ix<mig->nx-mig->tap+1;ix++,ix1+=mig->skipdx)R[ix]=(fftw_complex) wav->tzz[ix1*mod->naz+iz1]                   ;
							for(                          ;ix<mig->nx           ;ix++,ix1+=mig->skipdx)R[ix]=(fftw_complex)(wav->tzz[ix1*mod->naz+iz1]*tap[mig->nx-1-ix]);
						}else 	for(ix=0,ix1=mod->ioPx+mig->x1;ix<mig->nx           ;ix++,ix1+=mig->skipdx)R[ix]=(fftw_complex) wav->tzz[ix1*mod->naz+iz1]                   ;
						fftw_execute_dft(fftPlans->fft_1d_c2c_x,R,tmp); //Transform to Wavenumber Domain
						memset(&tmp[mig->nx/2+1],0,(mig->nx-mig->nx/2-1)*sizeof(fftw_complex)); //Mute Negative Wavenumbers
						fftw_execute_dft(fftPlans->ifft_1d_c2c_Kx,tmp,R); //Transform to Space Domain
						// Apply Imaging Condition
						for(ix=0;ix<mig->nx;ix++)mig->image[ix*mig->nz+iz]+=(float)creal(S[iz]*R[iz]);
						//Note: We multiply by 2*dt/nz^2 when writing to disk.
					}
					break;
			}
			fftw_free(S);fftw_free(R);fftw_free(tmp);
			free(mig->wav[mig->it].tzz);
			break;
	}

	return(0);
}
