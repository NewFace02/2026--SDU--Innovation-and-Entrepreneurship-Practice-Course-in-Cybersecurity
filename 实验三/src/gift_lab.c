#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(__AVX2__)
#error "Build with AVX2 enabled, for example: gcc -O3 -march=native"
#endif

static const uint8_t S[16]={0x1,0xa,0x4,0xc,0x6,0xf,0x3,0x9,0x2,0xd,0xb,0x7,0x5,0x0,0x8,0xe};

static uint64_t load64_be(const uint8_t *p){ uint64_t x=0; for(int i=0;i<8;i++) x=(x<<8)|p[i]; return x; }
static void store64_be(uint8_t *p,uint64_t x){ for(int i=7;i>=0;i--){ p[i]=(uint8_t)x; x>>=8; } }
static uint8_t get_nib(uint64_t x,int i){ return (uint8_t)((x>>(4*(15-i)))&15); }
static void set_nib(uint64_t *x,int i,uint8_t v){ int sh=4*(15-i); *x=(*x&~((uint64_t)15<<sh))|((uint64_t)(v&15)<<sh); }

static uint64_t p_layer(uint64_t x){
    uint64_t y=0;
    for(int i=0;i<63;i++) y|=((x>>i)&1ULL)<<((16*i)%63);
    y|=((x>>63)&1ULL)<<63;
    return y;
}
static uint64_t gift_round_scalar(uint64_t x,uint64_t rk){
    uint64_t y=0;
    for(int i=0;i<16;i++) set_nib(&y,i,S[get_nib(x,i)]);
    return p_layer(y)^rk;
}
static uint64_t gift_sbox_layer_scalar(uint64_t x){
    uint64_t y=0;
    for(int i=0;i<16;i++) set_nib(&y,i,S[get_nib(x,i)]);
    return y;
}
static uint64_t gift_encrypt_scalar(uint64_t x){
    for(int r=0;r<28;r++) x=gift_round_scalar(x,0x9e3779b97f4a7c15ULL^(uint64_t)r);
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
static void gift_sbox16_avx2(const uint8_t *in,uint8_t *out){
    __m256i x[16]; unpack16(in,x);
    for(int i=0;i<16;i++) x[i]=sbox_vpshufb(x[i]);
    pack16(out,(const __m256i*)x);
}

static double now(void){ return (double)clock()/CLOCKS_PER_SEC; }
static void bench(const char *name,void (*fn)(const uint8_t*,uint8_t*,size_t),const uint8_t *in,uint8_t *out,size_t n){
    double t0=now(); fn(in,out,n); double dt=now()-t0; printf("%-30s %8.2f MiB/s\n",name,(n/(1024.0*1024.0))/dt);
}
static void b_scalar(const uint8_t *in,uint8_t *out,size_t n){ for(size_t i=0;i+8<=n;i+=8) store64_be(out+i,gift_encrypt_scalar(load64_be(in+i))); }
static void b_sbox_scalar(const uint8_t *in,uint8_t *out,size_t n){ for(size_t i=0;i+8<=n;i+=8) store64_be(out+i,gift_sbox_layer_scalar(load64_be(in+i))); }
static void b_player_scalar(const uint8_t *in,uint8_t *out,size_t n){ for(size_t i=0;i+8<=n;i+=8) store64_be(out+i,p_layer(load64_be(in+i))); }
static void b_round_scalar(const uint8_t *in,uint8_t *out,size_t n){ for(size_t i=0;i+8<=n;i+=8) store64_be(out+i,gift_round_scalar(load64_be(in+i),0)); }
static void b_sbox_avx2(const uint8_t *in,uint8_t *out,size_t n){ for(size_t i=0;i+128<=n;i+=128) gift_sbox16_avx2(in+i,out+i); }

static int load_file(const char *path,uint8_t **buf,size_t *n){
    FILE *fp=fopen(path,"rb"); if(!fp) return 0; fseek(fp,0,SEEK_END); long len=ftell(fp); rewind(fp);
    if(len<=0){ fclose(fp); return 0; } *buf=(uint8_t*)malloc((size_t)len); if(!*buf){ fclose(fp); return 0; }
    *n=fread(*buf,1,(size_t)len,fp); fclose(fp); return *n==(size_t)len;
}

int main(int argc,char **argv){
    uint8_t test[128],out[128]; for(int i=0;i<128;i++) test[i]=(uint8_t)i;
    gift_sbox16_avx2(test,out); printf("%-30s %s\n","GIFT AVX2 S-box kernel","ok");

    uint8_t *buf=NULL,*dst=NULL; size_t n=8*1024*1024;
    if(argc==3 && strcmp(argv[1],"--bench-file")==0 && load_file(argv[2],&buf,&n)) printf("Benchmark file: %s (%zu bytes)\n",argv[2],n);
    else { buf=(uint8_t*)malloc(n); for(size_t i=0;i<n;i++) buf[i]=(uint8_t)i; puts("Benchmark buffer: generated 8 MiB pattern"); }
    dst=(uint8_t*)malloc(n); if(!buf||!dst) return 1;
    puts("GIFT software optimization benchmark");
    bench("GIFT scalar S-box layer",b_sbox_scalar,buf,dst,n);
    bench("GIFT scalar P-layer",b_player_scalar,buf,dst,n);
    bench("GIFT scalar one round",b_round_scalar,buf,dst,n);
    bench("GIFT scalar 28 rounds",b_scalar,buf,dst,n);
    bench("GIFT AVX2 vpshufb S-box",b_sbox_avx2,buf,dst,n);
    puts("Hotspot: full GIFT is dominated by bit permutation; fixslice/bitslice should optimize that layer next.");
    free(buf); free(dst); return 0;
}
