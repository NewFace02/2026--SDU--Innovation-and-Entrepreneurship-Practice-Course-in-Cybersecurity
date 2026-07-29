#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(__AVX2__)
#error "Build with AVX2 enabled, for example: gcc -O3 -march=native"
#endif

static const uint8_t S[16]={0xc,0x0,0xf,0xa,0x2,0xb,0x9,0x5,0x8,0x3,0xd,0x7,0x1,0xe,0x6,0x4};
static const uint8_t P[16]={5,0,1,4,7,12,3,8,13,6,9,2,15,10,11,14};

static uint64_t load64_be(const uint8_t *p){ uint64_t x=0; for(int i=0;i<8;i++) x=(x<<8)|p[i]; return x; }
static void store64_be(uint8_t *p,uint64_t x){ for(int i=7;i>=0;i--){ p[i]=(uint8_t)x; x>>=8; } }
static uint8_t get_nib(uint64_t x,int i){ return (uint8_t)((x>>(4*(15-i)))&15); }
static void set_nib(uint64_t *x,int i,uint8_t v){ int sh=4*(15-i); *x=(*x&~((uint64_t)15<<sh))|((uint64_t)(v&15)<<sh); }

static void toy_round_keys(uint8_t rk[36][8],const uint8_t key[10]){
    for(int r=0;r<36;r++) for(int i=0;i<8;i++) rk[r][i]=(uint8_t)((key[(r+i)%10]+r+i)&15);
}

static uint64_t twine_round_scalar(uint64_t x,const uint8_t rk[8]){
    uint8_t y[16],z[16];
    for(int i=0;i<16;i++) y[i]=get_nib(x,i);
    for(int j=0;j<8;j++) y[2*j+1]^=S[y[2*j]^rk[j]];
    for(int i=0;i<16;i++) z[P[i]]=y[i];
    uint64_t out=0; for(int i=0;i<16;i++) set_nib(&out,i,z[i]);
    return out;
}
static uint64_t twine_sbox_layer_scalar(uint64_t x,const uint8_t rk[8]){
    uint8_t y[16];
    for(int i=0;i<16;i++) y[i]=get_nib(x,i);
    for(int j=0;j<8;j++) y[2*j+1]^=S[y[2*j]^rk[j]];
    uint64_t out=0; for(int i=0;i<16;i++) set_nib(&out,i,y[i]);
    return out;
}
static uint64_t twine_perm_layer_scalar(uint64_t x){
    uint8_t y[16],z[16];
    for(int i=0;i<16;i++) y[i]=get_nib(x,i);
    for(int i=0;i<16;i++) z[P[i]]=y[i];
    uint64_t out=0; for(int i=0;i<16;i++) set_nib(&out,i,z[i]);
    return out;
}

static uint64_t twine_encrypt_scalar(uint64_t x,const uint8_t rk[36][8]){
    for(int r=0;r<36;r++) x=twine_round_scalar(x,rk[r]);
    return x;
}

static void unpack16(const uint8_t *in,__m256i x[16]){
    uint8_t tmp[16][32]; memset(tmp,0,sizeof(tmp));
    for(int b=0;b<16;b++){ uint64_t v=load64_be(in+8*b); for(int i=0;i<16;i++) tmp[i][b]=get_nib(v,i); }
    for(int i=0;i<16;i++) x[i]=_mm256_loadu_si256((const __m256i*)tmp[i]);
}
static void pack16(uint8_t *out,const __m256i x[16]){
    uint8_t tmp[16][32]; for(int i=0;i<16;i++) _mm256_storeu_si256((__m256i*)tmp[i],x[i]);
    for(int b=0;b<16;b++){ uint64_t v=0; for(int i=0;i<16;i++) set_nib(&v,i,tmp[i][b]); store64_be(out+8*b,v); }
}
static __m256i sbox_vpshufb(__m256i x){
    __m256i tab=_mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)S));
    return _mm256_shuffle_epi8(tab,_mm256_and_si256(x,_mm256_set1_epi8(15)));
}

static void twine_encrypt16_avx2(const uint8_t *in,uint8_t *out,const uint8_t rk[36][8]){
    __m256i x[16],y[16],t;
    unpack16(in,x);
    for(int r=0;r<36;r++){
        for(int i=0;i<16;i++) y[i]=x[i];
        for(int j=0;j<8;j++){
            t=_mm256_xor_si256(x[2*j],_mm256_set1_epi8((char)rk[r][j]));
            t=sbox_vpshufb(t);
            y[2*j+1]=_mm256_xor_si256(x[2*j+1],t);
        }
        for(int i=0;i<16;i++) x[P[i]]=y[i];
    }
    pack16(out,(const __m256i*)x);
}
static void twine_sbox16_avx2(const uint8_t *in,uint8_t *out,const uint8_t rk[8]){
    __m256i x[16],t; unpack16(in,x);
    for(int j=0;j<8;j++){
        t=_mm256_xor_si256(x[2*j],_mm256_set1_epi8((char)rk[j]));
        t=sbox_vpshufb(t);
        x[2*j+1]=_mm256_xor_si256(x[2*j+1],t);
    }
    pack16(out,(const __m256i*)x);
}

static double now(void){ return (double)clock()/CLOCKS_PER_SEC; }
static void bench(const char *name,void (*fn)(const uint8_t*,uint8_t*,size_t),const uint8_t *in,uint8_t *out,size_t n){
    double t0=now(); fn(in,out,n); double dt=now()-t0; printf("%-30s %8.2f MiB/s\n",name,(n/(1024.0*1024.0))/dt);
}

static uint8_t RK[36][8];
static void b_scalar(const uint8_t *in,uint8_t *out,size_t n){ for(size_t i=0;i+8<=n;i+=8) store64_be(out+i,twine_encrypt_scalar(load64_be(in+i),RK)); }
static void b_sbox_scalar(const uint8_t *in,uint8_t *out,size_t n){ for(size_t i=0;i+8<=n;i+=8) store64_be(out+i,twine_sbox_layer_scalar(load64_be(in+i),RK[0])); }
static void b_perm_scalar(const uint8_t *in,uint8_t *out,size_t n){ for(size_t i=0;i+8<=n;i+=8) store64_be(out+i,twine_perm_layer_scalar(load64_be(in+i))); }
static void b_round_scalar(const uint8_t *in,uint8_t *out,size_t n){ for(size_t i=0;i+8<=n;i+=8) store64_be(out+i,twine_round_scalar(load64_be(in+i),RK[0])); }
static void b_avx2(const uint8_t *in,uint8_t *out,size_t n){ for(size_t i=0;i+128<=n;i+=128) twine_encrypt16_avx2(in+i,out+i,RK); }
static void b_sbox_avx2(const uint8_t *in,uint8_t *out,size_t n){ for(size_t i=0;i+128<=n;i+=128) twine_sbox16_avx2(in+i,out+i,RK[0]); }

static int load_file(const char *path,uint8_t **buf,size_t *n){
    FILE *fp=fopen(path,"rb"); if(!fp) return 0; fseek(fp,0,SEEK_END); long len=ftell(fp); rewind(fp);
    if(len<=0){ fclose(fp); return 0; } *buf=(uint8_t*)malloc((size_t)len); if(!*buf){ fclose(fp); return 0; }
    *n=fread(*buf,1,(size_t)len,fp); fclose(fp); return *n==(size_t)len;
}

int main(int argc,char **argv){
    uint8_t key[10]={0,1,2,3,4,5,6,7,8,9}; toy_round_keys(RK,key);
    uint8_t test[128],a[128],b[128]; for(int i=0;i<128;i++) test[i]=(uint8_t)i;
    b_scalar(test,a,128); twine_encrypt16_avx2(test,b,RK);
    int ok=memcmp(a,b,128)==0; printf("%-30s %s\n","TWINE scalar vs AVX2",ok?"ok":"FAIL");

    uint8_t *buf=NULL,*out=NULL; size_t n=8*1024*1024;
    if(argc==3 && strcmp(argv[1],"--bench-file")==0 && load_file(argv[2],&buf,&n)) printf("Benchmark file: %s (%zu bytes)\n",argv[2],n);
    else { buf=(uint8_t*)malloc(n); for(size_t i=0;i<n;i++) buf[i]=(uint8_t)i; puts("Benchmark buffer: generated 8 MiB pattern"); }
    out=(uint8_t*)malloc(n); if(!buf||!out) return 1;
    puts("TWINE software optimization benchmark");
    bench("TWINE scalar S-box layer",b_sbox_scalar,buf,out,n);
    bench("TWINE scalar perm layer",b_perm_scalar,buf,out,n);
    bench("TWINE scalar one round",b_round_scalar,buf,out,n);
    bench("TWINE scalar 36 rounds",b_scalar,buf,out,n);
    bench("TWINE AVX2 vpshufb S-box",b_sbox_avx2,buf,out,n);
    bench("TWINE AVX2 vpshufb",b_avx2,buf,out,n);
    free(buf); free(out); return ok?0:1;
}
