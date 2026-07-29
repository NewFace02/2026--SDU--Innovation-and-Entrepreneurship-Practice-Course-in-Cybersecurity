#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(__AVX2__)
#error "Build with AVX2 enabled, for example: gcc -O3 -march=native"
#endif

typedef struct { uint32_t rk[32]; } sm4_key;

static const uint8_t sm4_sbox[256] = {
0xd6,0x90,0xe9,0xfe,0xcc,0xe1,0x3d,0xb7,0x16,0xb6,0x14,0xc2,0x28,0xfb,0x2c,0x05,
0x2b,0x67,0x9a,0x76,0x2a,0xbe,0x04,0xc3,0xaa,0x44,0x13,0x26,0x49,0x86,0x06,0x99,
0x9c,0x42,0x50,0xf4,0x91,0xef,0x98,0x7a,0x33,0x54,0x0b,0x43,0xed,0xcf,0xac,0x62,
0xe4,0xb3,0x1c,0xa9,0xc9,0x08,0xe8,0x95,0x80,0xdf,0x94,0xfa,0x75,0x8f,0x3f,0xa6,
0x47,0x07,0xa7,0xfc,0xf3,0x73,0x17,0xba,0x83,0x59,0x3c,0x19,0xe6,0x85,0x4f,0xa8,
0x68,0x6b,0x81,0xb2,0x71,0x64,0xda,0x8b,0xf8,0xeb,0x0f,0x4b,0x70,0x56,0x9d,0x35,
0x1e,0x24,0x0e,0x5e,0x63,0x58,0xd1,0xa2,0x25,0x22,0x7c,0x3b,0x01,0x21,0x78,0x87,
0xd4,0x00,0x46,0x57,0x9f,0xd3,0x27,0x52,0x4c,0x36,0x02,0xe7,0xa0,0xc4,0xc8,0x9e,
0xea,0xbf,0x8a,0xd2,0x40,0xc7,0x38,0xb5,0xa3,0xf7,0xf2,0xce,0xf9,0x61,0x15,0xa1,
0xe0,0xae,0x5d,0xa4,0x9b,0x34,0x1a,0x55,0xad,0x93,0x32,0x30,0xf5,0x8c,0xb1,0xe3,
0x1d,0xf6,0xe2,0x2e,0x82,0x66,0xca,0x60,0xc0,0x29,0x23,0xab,0x0d,0x53,0x4e,0x6f,
0xd5,0xdb,0x37,0x45,0xde,0xfd,0x8e,0x2f,0x03,0xff,0x6a,0x72,0x6d,0x6c,0x5b,0x51,
0x8d,0x1b,0xaf,0x92,0xbb,0xdd,0xbc,0x7f,0x11,0xd9,0x5c,0x41,0x1f,0x10,0x5a,0xd8,
0x0a,0xc1,0x31,0x88,0xa5,0xcd,0x7b,0xbd,0x2d,0x74,0xd0,0x12,0xb8,0xe5,0xb4,0xb0,
0x89,0x69,0x97,0x4a,0x0c,0x96,0x77,0x7e,0x65,0xb9,0xf1,0x09,0xc5,0x6e,0xc6,0x84,
0x18,0xf0,0x7d,0xec,0x3a,0xdc,0x4d,0x20,0x79,0xee,0x5f,0x3e,0xd7,0xcb,0x39,0x48};

static const uint32_t FK[4] = {0xa3b1bac6,0x56aa3350,0x677d9197,0xb27022dc};
static const uint32_t CK[32] = {
0x00070e15,0x1c232a31,0x383f464d,0x545b6269,0x70777e85,0x8c939aa1,0xa8afb6bd,0xc4cbd2d9,
0xe0e7eef5,0xfc030a11,0x181f262d,0x343b4249,0x50575e65,0x6c737a81,0x888f969d,0xa4abb2b9,
0xc0c7ced5,0xdce3eaf1,0xf8ff060d,0x141b2229,0x30373e45,0x4c535a61,0x686f767d,0x848b9299,
0xa0a7aeb5,0xbcc3cad1,0xd8dfe6ed,0xf4fb0209,0x10171e25,0x2c333a41,0x484f565d,0x646b7279};

static uint32_t T0[256], T1[256], T2[256], T3[256];
static uint64_t T2box[256];

static uint32_t rol32(uint32_t x,int n){ return (x<<n)|(x>>(32-n)); }
static uint32_t load_be32(const uint8_t *p){ return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }
static void store_be32(uint8_t *p,uint32_t x){ p[0]=x>>24; p[1]=x>>16; p[2]=x>>8; p[3]=x; }
static uint32_t tau(uint32_t x){ return ((uint32_t)sm4_sbox[x>>24]<<24)|((uint32_t)sm4_sbox[(x>>16)&255]<<16)|((uint32_t)sm4_sbox[(x>>8)&255]<<8)|sm4_sbox[x&255]; }
static uint32_t L(uint32_t x){ return x^rol32(x,2)^rol32(x,10)^rol32(x,18)^rol32(x,24); }
static uint32_t Lp(uint32_t x){ return x^rol32(x,13)^rol32(x,23); }
static uint32_t T_ref(uint32_t x){ return L(tau(x)); }
static uint32_t Tp(uint32_t x){ return Lp(tau(x)); }

static void sm4_tables_init(void){
    for(int i=0;i<256;i++){
        uint32_t v=L((uint32_t)sm4_sbox[i]<<24);
        T0[i]=v; T1[i]=rol32(v,8); T2[i]=rol32(v,16); T3[i]=rol32(v,24);
        T2box[i]=((uint64_t)v<<32)|v;
    }
}

static uint32_t T_4table(uint32_t x){ return T0[x>>24]^T3[(x>>16)&255]^T2[(x>>8)&255]^T1[x&255]; }
static uint32_t T_tbox2(uint32_t x){
    uint32_t a=(uint32_t)(T2box[x>>24]>>32);
    uint32_t b=(uint32_t)(T2box[(x>>16)&255]>>8);
    uint32_t c=(uint32_t)(T2box[(x>>8)&255]>>16);
    uint32_t d=(uint32_t)(T2box[x&255]>>24);
    return a^b^c^d;
}

static void sm4_key_expand(sm4_key *k,const uint8_t key[16]){
    uint32_t K[36];
    for(int i=0;i<4;i++) K[i]=load_be32(key+4*i)^FK[i];
    for(int i=0;i<32;i++){ K[i+4]=K[i]^Tp(K[i+1]^K[i+2]^K[i+3]^CK[i]); k->rk[i]=K[i+4]; }
}

static void sm4_encrypt_ref(const sm4_key *k,const uint8_t in[16],uint8_t out[16]){
    uint32_t X[36]; for(int i=0;i<4;i++) X[i]=load_be32(in+4*i);
    for(int i=0;i<32;i++) X[i+4]=X[i]^T_ref(X[i+1]^X[i+2]^X[i+3]^k->rk[i]);
    for(int i=0;i<4;i++) store_be32(out+4*i,X[35-i]);
}

static void sm4_encrypt_4table(const sm4_key *k,const uint8_t in[16],uint8_t out[16]){
    uint32_t x0=load_be32(in),x1=load_be32(in+4),x2=load_be32(in+8),x3=load_be32(in+12),x4;
    for(int i=0;i<32;i++){ x4=x0^T_4table(x1^x2^x3^k->rk[i]); x0=x1; x1=x2; x2=x3; x3=x4; }
    store_be32(out,x3); store_be32(out+4,x2); store_be32(out+8,x1); store_be32(out+12,x0);
}

static void sm4_encrypt_tbox2(const sm4_key *k,const uint8_t in[16],uint8_t out[16]){
    uint32_t x0=load_be32(in),x1=load_be32(in+4),x2=load_be32(in+8),x3=load_be32(in+12),x4;
    for(int i=0;i<32;i++){ x4=x0^T_tbox2(x1^x2^x3^k->rk[i]); x0=x1; x1=x2; x2=x3; x3=x4; }
    store_be32(out,x3); store_be32(out+4,x2); store_be32(out+8,x1); store_be32(out+12,x0);
}

static void sm4_crypt_tbox2_order(const sm4_key *k,const uint8_t in[16],uint8_t out[16],int decrypt){
    uint32_t x0=load_be32(in),x1=load_be32(in+4),x2=load_be32(in+8),x3=load_be32(in+12),x4;
    for(int i=0;i<32;i++){ uint32_t rk=decrypt?k->rk[31-i]:k->rk[i]; x4=x0^T_tbox2(x1^x2^x3^rk); x0=x1; x1=x2; x2=x3; x3=x4; }
    store_be32(out,x3); store_be32(out+4,x2); store_be32(out+8,x1); store_be32(out+12,x0);
}

static void inc_be128(uint8_t c[16]){ for(int i=15;i>=0;i--) if(++c[i]) break; }
static void sm4_ctr_tbox2(const sm4_key *k,uint8_t ctr[16],const uint8_t *in,uint8_t *out,size_t n){
    uint8_t stream[16];
    while(n){ sm4_encrypt_tbox2(k,ctr,stream); size_t m=n<16?n:16; for(size_t i=0;i<m;i++) out[i]=in[i]^stream[i]; in+=m; out+=m; n-=m; inc_be128(ctr); }
}

static __m256i gather_t(const uint32_t *tab,__m256i idx){ return _mm256_i32gather_epi32((const int*)tab,idx,4); }
static void sm4_encrypt8_avx2(const sm4_key *k,const uint8_t in[8][16],uint8_t out[8][16]){
    uint32_t a[8],b[8],c[8],d[8];
    for(int i=0;i<8;i++){ a[i]=load_be32(in[i]); b[i]=load_be32(in[i]+4); c[i]=load_be32(in[i]+8); d[i]=load_be32(in[i]+12); }
    __m256i x0=_mm256_loadu_si256((const __m256i*)a), x1=_mm256_loadu_si256((const __m256i*)b);
    __m256i x2=_mm256_loadu_si256((const __m256i*)c), x3=_mm256_loadu_si256((const __m256i*)d);
    const __m256i mask=_mm256_set1_epi32(255);
    for(int r=0;r<32;r++){
        __m256i y=x1^x2^x3^_mm256_set1_epi32((int)k->rk[r]);
        __m256i i0=_mm256_and_si256(_mm256_srli_epi32(y,24),mask);
        __m256i i1=_mm256_and_si256(_mm256_srli_epi32(y,16),mask);
        __m256i i2=_mm256_and_si256(_mm256_srli_epi32(y,8),mask);
        __m256i i3=_mm256_and_si256(y,mask);
        __m256i t=gather_t(T0,i0)^gather_t(T3,i1)^gather_t(T2,i2)^gather_t(T1,i3);
        __m256i x4=x0^t; x0=x1; x1=x2; x2=x3; x3=x4;
    }
    _mm256_storeu_si256((__m256i*)a,x3); _mm256_storeu_si256((__m256i*)b,x2);
    _mm256_storeu_si256((__m256i*)c,x1); _mm256_storeu_si256((__m256i*)d,x0);
    for(int i=0;i<8;i++){ store_be32(out[i],a[i]); store_be32(out[i]+4,b[i]); store_be32(out[i]+8,c[i]); store_be32(out[i]+12,d[i]); }
}

static void sm4_ctr8_avx2(const sm4_key *k,uint8_t ctr[16],const uint8_t *in,uint8_t *out,size_t n){
    uint8_t counters[8][16], stream[8][16];
    while(n>=128){
        for(int i=0;i<8;i++){ memcpy(counters[i],ctr,16); inc_be128(ctr); }
        sm4_encrypt8_avx2(k,(const uint8_t (*)[16])counters,stream);
        for(int i=0;i<8;i++){ __m128i p=_mm_loadu_si128((const __m128i*)(in+16*i)); __m128i s=_mm_loadu_si128((const __m128i*)stream[i]); _mm_storeu_si128((__m128i*)(out+16*i),p^s); }
        in+=128; out+=128; n-=128;
    }
    sm4_ctr_tbox2(k,ctr,in,out,n);
}

static void xor16(uint8_t a[16],const uint8_t b[16]){ for(int i=0;i<16;i++) a[i]^=b[i]; }
static void ghash_mulx(uint8_t x[16]){ int lsb=x[15]&1; for(int j=15;j>0;j--) x[j]=(x[j]>>1)|((x[j-1]&1)<<7); x[0]>>=1; if(lsb) x[0]^=0xe1; }
static void ghash_mulx4(uint8_t x[16]){ for(int i=0;i<4;i++) ghash_mulx(x); }
static void ghash_make_4bit_table(uint8_t tab[16][16],const uint8_t h[16]){
    memset(tab,0,16*16);
    for(int n=0;n<16;n++){ uint8_t z[16]={0}; for(int b=3;b>=0;b--){ ghash_mulx(z); if((n>>b)&1) xor16(z,h); } memcpy(tab[n],z,16); }
}
static void ghash_mul_4bit(uint8_t x[16],const uint8_t tab[16][16]){
    uint8_t z[16]={0};
    for(int i=0;i<16;i++){ ghash_mulx4(z); xor16(z,tab[x[i]>>4]); ghash_mulx4(z); xor16(z,tab[x[i]&15]); }
    memcpy(x,z,16);
}
static void ghash_update_4bit(uint8_t y[16],const uint8_t tab[16][16],const uint8_t *p,size_t n){
    uint8_t b[16]; while(n){ memset(b,0,16); size_t m=n<16?n:16; memcpy(b,p,m); xor16(y,b); ghash_mul_4bit(y,tab); p+=m; n-=m; }
}
static void put64be(uint8_t *p,uint64_t x){ for(int i=7;i>=0;i--){ p[i]=x; x>>=8; } }
static void sm4_gcm_encrypt(const sm4_key *k,const uint8_t iv[12],const uint8_t *aad,size_t alen,const uint8_t *pt,uint8_t *ct,size_t n,uint8_t tag[16]){
    uint8_t zero[16]={0}, h[16], htab[16][16], y[16]={0}, j0[16]={0}, ctr[16], lenb[16];
    sm4_encrypt_tbox2(k,zero,h); ghash_make_4bit_table(htab,h);
    memcpy(j0,iv,12); j0[15]=1; memcpy(ctr,j0,16); inc_be128(ctr);
    sm4_ctr8_avx2(k,ctr,pt,ct,n);
    ghash_update_4bit(y,htab,aad,alen); ghash_update_4bit(y,htab,ct,n);
    memset(lenb,0,16); put64be(lenb,(uint64_t)alen*8); put64be(lenb+8,(uint64_t)n*8); xor16(y,lenb); ghash_mul_4bit(y,htab);
    sm4_encrypt_tbox2(k,j0,tag); xor16(tag,y);
}

static void gf_mul_x(uint8_t t[16]){ uint8_t carry=0; for(int i=0;i<16;i++){ uint8_t ncarry=t[i]>>7; t[i]=(uint8_t)((t[i]<<1)|carry); carry=ncarry; } if(carry) t[15]^=0x87; }
static void sm4_xts_crypt(const sm4_key *data,const sm4_key *tweak,const uint8_t iv[16],const uint8_t *in,uint8_t *out,size_t n,int decrypt){
    uint8_t tw[16]; sm4_encrypt_tbox2(tweak,iv,tw);
    for(size_t off=0;off+16<=n;off+=16){ uint8_t b[16]; for(int i=0;i<16;i++) b[i]=in[off+i]^tw[i]; sm4_crypt_tbox2_order(data,b,b,decrypt); for(int i=0;i<16;i++) out[off+i]=b[i]^tw[i]; gf_mul_x(tw); }
}

typedef struct { sm4_key key; uint8_t *buf,*out; } bench_ctx;
static double now(void){ return (double)clock()/CLOCKS_PER_SEC; }
static void bench(const char *name,void (*fn)(void*,size_t),void *ctx,size_t bytes){ double t0=now(); fn(ctx,bytes); double dt=now()-t0; printf("%-28s %8.2f MiB/s\n",name,(bytes/(1024.0*1024.0))/dt); }
static void b_ref(void *v,size_t n){ bench_ctx*c=v; uint8_t ctr[16]={0}; uint8_t stream[16]; const uint8_t *in=c->buf; uint8_t *out=c->out; while(n){ sm4_encrypt_ref(&c->key,ctr,stream); size_t m=n<16?n:16; for(size_t i=0;i<m;i++) out[i]=in[i]^stream[i]; in+=m; out+=m; n-=m; inc_be128(ctr); } }
static void b_4table(void *v,size_t n){ bench_ctx*c=v; uint8_t ctr[16]={0}; uint8_t stream[16]; const uint8_t *in=c->buf; uint8_t *out=c->out; while(n){ sm4_encrypt_4table(&c->key,ctr,stream); size_t m=n<16?n:16; for(size_t i=0;i<m;i++) out[i]=in[i]^stream[i]; in+=m; out+=m; n-=m; inc_be128(ctr); } }
static void b_tbox2(void *v,size_t n){ bench_ctx*c=v; uint8_t ctr[16]={0}; sm4_ctr_tbox2(&c->key,ctr,c->buf,c->out,n); }
static void b_avx2(void *v,size_t n){ bench_ctx*c=v; uint8_t ctr[16]={0}; sm4_ctr8_avx2(&c->key,ctr,c->buf,c->out,n); }
static void b_gcm(void *v,size_t n){ bench_ctx*c=v; uint8_t iv[12]={0},tag[16]; sm4_gcm_encrypt(&c->key,iv,NULL,0,c->buf,c->out,n,tag); }
static void b_xts(void *v,size_t n){ bench_ctx*c=v; uint8_t iv[16]={0}; sm4_xts_crypt(&c->key,&c->key,iv,c->buf,c->out,n&~(size_t)15,0); }

static int load_file(const char *path,uint8_t **buf,size_t *n){
    FILE *fp=fopen(path,"rb"); if(!fp) return 0;
    fseek(fp,0,SEEK_END); long len=ftell(fp); rewind(fp); if(len<=0){ fclose(fp); return 0; }
    *buf=(uint8_t*)malloc((size_t)len); if(!*buf){ fclose(fp); return 0; }
    *n=fread(*buf,1,(size_t)len,fp); fclose(fp); return *n==(size_t)len;
}

static int check(const char *name,const uint8_t *a,const uint8_t *b,size_t n){ int ok=memcmp(a,b,n)==0; printf("%-28s %s\n",name,ok?"ok":"FAIL"); return ok; }

int main(int argc,char **argv){
    sm4_tables_init();
    const uint8_t key[16]={0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10};
    const uint8_t pt[16]={0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10};
    const uint8_t exp[16]={0x68,0x1e,0xdf,0x34,0xd2,0x06,0x96,0x5e,0x86,0xb3,0xe9,0x4f,0x53,0x6e,0x42,0x46};
    sm4_key sk; sm4_key_expand(&sk,key);
    uint8_t a[16],b[16],c[16]; sm4_encrypt_ref(&sk,pt,a); sm4_encrypt_4table(&sk,pt,b); sm4_encrypt_tbox2(&sk,pt,c);
    int ok=1; ok&=check("SM4 reference",a,exp,16); ok&=check("SM4 4-table",b,exp,16); ok&=check("SM4 T-box2",c,exp,16);
    uint8_t in8[8][16], out8[8][16], out1[8][16]; for(int i=0;i<8;i++){ memcpy(in8[i],pt,16); in8[i][15]^=(uint8_t)i; sm4_encrypt_tbox2(&sk,in8[i],out1[i]); }
    sm4_encrypt8_avx2(&sk,(const uint8_t (*)[16])in8,out8); ok&=check("SM4 AVX2 8-way",(const uint8_t*)out8,(const uint8_t*)out1,128);
    uint8_t xp[32],xc[32],xr[32],iv[16]={0}; for(int i=0;i<32;i++) xp[i]=(uint8_t)i;
    sm4_xts_crypt(&sk,&sk,iv,xp,xc,32,0); sm4_xts_crypt(&sk,&sk,iv,xc,xr,32,1); ok&=check("SM4 XTS round trip",xp,xr,32);
    uint8_t *buf=NULL,*out=NULL; size_t n=8*1024*1024; const char *bench_file=NULL;
    if(argc==3 && strcmp(argv[1],"--bench-file")==0) bench_file=argv[2];
    if(bench_file && load_file(bench_file,&buf,&n)) printf("\nBenchmark file: %s (%zu bytes)\n",bench_file,n);
    else { buf=(uint8_t*)malloc(n); for(size_t i=0;i<n;i++) buf[i]=(uint8_t)i; puts("\nBenchmark buffer: generated 8 MiB pattern"); }
    out=(uint8_t*)malloc(n); if(!buf||!out){ fprintf(stderr,"allocation failed\n"); return 1; }
    bench_ctx bc={sk,buf,out};
    puts("SM4 mode benchmark"); bench("CTR reference",&b_ref,&bc,n); bench("CTR 4-table",&b_4table,&bc,n); bench("CTR T-box2",&b_tbox2,&bc,n); bench("CTR AVX2 8-way gather",&b_avx2,&bc,n); bench("GCM CTR+AVX2+4bit GHASH",&b_gcm,&bc,n); bench("XTS T-box2+tweak",&b_xts,&bc,n);
    free(buf); free(out); return ok?0:1;
}
