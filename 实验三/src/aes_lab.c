#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(__AES__) || !defined(__SSSE3__) || !defined(__AVX2__)
#error "Build with AES-NI, SSSE3 and AVX2 enabled, for example: gcc -O3 -march=native"
#endif

typedef struct { uint32_t rk[44]; } aes128_sw_key;
typedef struct { __m128i enc[11], dec[11]; } aes128_ni_key;

static const uint8_t sbox[256] = {
0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16};
static const uint8_t invsbox[256] __attribute__((unused)) = {
0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d};
static const uint8_t rcon[10]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36};

static uint32_t Te0[256], Te1[256], Te2[256], Te3[256];

static uint8_t xtime(uint8_t x){ return (uint8_t)((x<<1) ^ ((x>>7)*0x1b)); }
static uint8_t gmul(uint8_t a,uint8_t b){ uint8_t r=0; for(int i=0;i<8;i++){ if(b&1) r^=a; a=xtime(a); b>>=1; } return r; }
static uint32_t pack_be(const uint8_t *p){ return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }
static void unpack_be(uint32_t x,uint8_t *p){ p[0]=x>>24; p[1]=x>>16; p[2]=x>>8; p[3]=x; }
static uint32_t rotr32(uint32_t x,int n){ return (x>>n)|(x<<(32-n)); }
static uint32_t rot_word(uint32_t x){ return (x<<8)|(x>>24); }
static uint32_t sub_word(uint32_t x){ return ((uint32_t)sbox[x>>24]<<24)|((uint32_t)sbox[(x>>16)&255]<<16)|((uint32_t)sbox[(x>>8)&255]<<8)|sbox[x&255]; }

static void aes_tables_init(void){
    for(int i=0;i<256;i++){
        uint8_t s=sbox[i], s2=gmul(s,2), s3=s2^s;
        Te0[i]=((uint32_t)s2<<24)|((uint32_t)s<<16)|((uint32_t)s<<8)|s3;
        Te1[i]=((uint32_t)s3<<24)|((uint32_t)s2<<16)|((uint32_t)s<<8)|s;
        Te2[i]=((uint32_t)s<<24)|((uint32_t)s3<<16)|((uint32_t)s2<<8)|s;
        Te3[i]=((uint32_t)s<<24)|((uint32_t)s<<16)|((uint32_t)s3<<8)|s2;
    }
}

static void aes128_key_expand_sw(aes128_sw_key *k,const uint8_t key[16]){
    for(int i=0;i<4;i++) k->rk[i]=pack_be(key+4*i);
    for(int i=4;i<44;i++){
        uint32_t t=k->rk[i-1];
        if((i&3)==0) t=sub_word(rot_word(t)) ^ ((uint32_t)rcon[i/4-1]<<24);
        k->rk[i]=k->rk[i-4]^t;
    }
}

static void add_round_key(uint8_t s[16],const uint32_t *rk){ for(int c=0;c<4;c++){ uint32_t w=rk[c]; s[4*c]^=w>>24; s[4*c+1]^=w>>16; s[4*c+2]^=w>>8; s[4*c+3]^=w; } }
static void sub_bytes(uint8_t s[16]){ for(int i=0;i<16;i++) s[i]=sbox[s[i]]; }
static void shift_rows(uint8_t s[16]){ uint8_t t[16]={s[0],s[5],s[10],s[15],s[4],s[9],s[14],s[3],s[8],s[13],s[2],s[7],s[12],s[1],s[6],s[11]}; memcpy(s,t,16); }
static void mix_columns(uint8_t s[16]){ for(int c=0;c<4;c++){ uint8_t *p=s+4*c,a=p[0],b=p[1],d=p[2],e=p[3],x=a^b^d^e; p[0]^=x^xtime(a^b); p[1]^=x^xtime(b^d); p[2]^=x^xtime(d^e); p[3]^=x^xtime(e^a); } }

static void aes128_encrypt_ref(const aes128_sw_key *k,const uint8_t in[16],uint8_t out[16]){
    uint8_t s[16]; memcpy(s,in,16); add_round_key(s,k->rk);
    for(int r=1;r<10;r++){ sub_bytes(s); shift_rows(s); mix_columns(s); add_round_key(s,k->rk+4*r); }
    sub_bytes(s); shift_rows(s); add_round_key(s,k->rk+40); memcpy(out,s,16);
}

static void aes128_encrypt_ttable(const aes128_sw_key *k,const uint8_t in[16],uint8_t out[16]){
    uint32_t s0=pack_be(in)^k->rk[0], s1=pack_be(in+4)^k->rk[1], s2=pack_be(in+8)^k->rk[2], s3=pack_be(in+12)^k->rk[3],t0,t1,t2,t3;
    for(int r=1;r<10;r++){
        t0=Te0[s0>>24]^Te1[(s1>>16)&255]^Te2[(s2>>8)&255]^Te3[s3&255]^k->rk[4*r];
        t1=Te0[s1>>24]^Te1[(s2>>16)&255]^Te2[(s3>>8)&255]^Te3[s0&255]^k->rk[4*r+1];
        t2=Te0[s2>>24]^Te1[(s3>>16)&255]^Te2[(s0>>8)&255]^Te3[s1&255]^k->rk[4*r+2];
        t3=Te0[s3>>24]^Te1[(s0>>16)&255]^Te2[(s1>>8)&255]^Te3[s2&255]^k->rk[4*r+3];
        s0=t0; s1=t1; s2=t2; s3=t3;
    }
    uint8_t st[16]; unpack_be(s0,st); unpack_be(s1,st+4); unpack_be(s2,st+8); unpack_be(s3,st+12);
    uint8_t last[16]={sbox[st[0]],sbox[st[5]],sbox[st[10]],sbox[st[15]],sbox[st[4]],sbox[st[9]],sbox[st[14]],sbox[st[3]],sbox[st[8]],sbox[st[13]],sbox[st[2]],sbox[st[7]],sbox[st[12]],sbox[st[1]],sbox[st[6]],sbox[st[11]]};
    add_round_key(last,k->rk+40); memcpy(out,last,16);
}

static void aes128_encrypt_t2(const aes128_sw_key *k,const uint8_t in[16],uint8_t out[16]){
    uint32_t s0=pack_be(in)^k->rk[0], s1=pack_be(in+4)^k->rk[1], s2=pack_be(in+8)^k->rk[2], s3=pack_be(in+12)^k->rk[3],t0,t1,t2,t3;
    for(int r=1;r<10;r++){
        t0=Te0[s0>>24]^Te1[(s1>>16)&255]^rotr32(Te0[(s2>>8)&255],16)^rotr32(Te1[s3&255],16)^k->rk[4*r];
        t1=Te0[s1>>24]^Te1[(s2>>16)&255]^rotr32(Te0[(s3>>8)&255],16)^rotr32(Te1[s0&255],16)^k->rk[4*r+1];
        t2=Te0[s2>>24]^Te1[(s3>>16)&255]^rotr32(Te0[(s0>>8)&255],16)^rotr32(Te1[s1&255],16)^k->rk[4*r+2];
        t3=Te0[s3>>24]^Te1[(s0>>16)&255]^rotr32(Te0[(s1>>8)&255],16)^rotr32(Te1[s2&255],16)^k->rk[4*r+3];
        s0=t0; s1=t1; s2=t2; s3=t3;
    }
    uint8_t st[16]; unpack_be(s0,st); unpack_be(s1,st+4); unpack_be(s2,st+8); unpack_be(s3,st+12);
    uint8_t last[16]={sbox[st[0]],sbox[st[5]],sbox[st[10]],sbox[st[15]],sbox[st[4]],sbox[st[9]],sbox[st[14]],sbox[st[3]],sbox[st[8]],sbox[st[13]],sbox[st[2]],sbox[st[7]],sbox[st[12]],sbox[st[1]],sbox[st[6]],sbox[st[11]]};
    add_round_key(last,k->rk+40); memcpy(out,last,16);
}

#define AES_128_ASSIST(x,c) do{ __m128i y=_mm_aeskeygenassist_si128((x),(c)); y=_mm_shuffle_epi32(y,0xff); (x)^=_mm_slli_si128((x),4); (x)^=_mm_slli_si128((x),4); (x)^=_mm_slli_si128((x),4); (x)^=y; }while(0)
static void aes128_key_expand_ni(aes128_ni_key *k,const uint8_t key[16]){
    k->enc[0]=_mm_loadu_si128((const __m128i*)key);
    k->enc[1]=k->enc[0]; AES_128_ASSIST(k->enc[1],0x01);
    k->enc[2]=k->enc[1]; AES_128_ASSIST(k->enc[2],0x02);
    k->enc[3]=k->enc[2]; AES_128_ASSIST(k->enc[3],0x04);
    k->enc[4]=k->enc[3]; AES_128_ASSIST(k->enc[4],0x08);
    k->enc[5]=k->enc[4]; AES_128_ASSIST(k->enc[5],0x10);
    k->enc[6]=k->enc[5]; AES_128_ASSIST(k->enc[6],0x20);
    k->enc[7]=k->enc[6]; AES_128_ASSIST(k->enc[7],0x40);
    k->enc[8]=k->enc[7]; AES_128_ASSIST(k->enc[8],0x80);
    k->enc[9]=k->enc[8]; AES_128_ASSIST(k->enc[9],0x1b);
    k->enc[10]=k->enc[9]; AES_128_ASSIST(k->enc[10],0x36);
    k->dec[0]=k->enc[10]; for(int i=1;i<10;i++) k->dec[i]=_mm_aesimc_si128(k->enc[10-i]); k->dec[10]=k->enc[0];
}
static __m128i aes128_encrypt_block_ni(const aes128_ni_key *k,__m128i x){ x^=k->enc[0]; for(int r=1;r<10;r++) x=_mm_aesenc_si128(x,k->enc[r]); return _mm_aesenclast_si128(x,k->enc[10]); }
static __m128i aes128_decrypt_block_ni(const aes128_ni_key *k,__m128i x){ x^=k->dec[0]; for(int r=1;r<10;r++) x=_mm_aesdec_si128(x,k->dec[r]); return _mm_aesdeclast_si128(x,k->dec[10]); }
static void aes128_encrypt_ni(const aes128_ni_key *k,const uint8_t in[16],uint8_t out[16]){ _mm_storeu_si128((__m128i*)out,aes128_encrypt_block_ni(k,_mm_loadu_si128((const __m128i*)in))); }

static void inc_be128(uint8_t c[16]){ for(int i=15;i>=0;i--) if(++c[i]) break; }
static void ctr_crypt_ref(const aes128_sw_key *k,uint8_t ctr[16],const uint8_t *in,uint8_t *out,size_t n){ uint8_t stream[16]; while(n){ aes128_encrypt_ref(k,ctr,stream); size_t m=n<16?n:16; for(size_t i=0;i<m;i++) out[i]=in[i]^stream[i]; in+=m; out+=m; n-=m; inc_be128(ctr); } }
static void ctr_crypt_ttable(const aes128_sw_key *k,uint8_t ctr[16],const uint8_t *in,uint8_t *out,size_t n){ uint8_t stream[16]; while(n){ aes128_encrypt_ttable(k,ctr,stream); size_t m=n<16?n:16; for(size_t i=0;i<m;i++) out[i]=in[i]^stream[i]; in+=m; out+=m; n-=m; inc_be128(ctr); } }
static void ctr_crypt_t2(const aes128_sw_key *k,uint8_t ctr[16],const uint8_t *in,uint8_t *out,size_t n){ uint8_t stream[16]; while(n){ aes128_encrypt_t2(k,ctr,stream); size_t m=n<16?n:16; for(size_t i=0;i<m;i++) out[i]=in[i]^stream[i]; in+=m; out+=m; n-=m; inc_be128(ctr); } }
static void ctr_crypt_ni1(const aes128_ni_key *k,uint8_t ctr[16],const uint8_t *in,uint8_t *out,size_t n){ uint8_t stream[16]; while(n){ aes128_encrypt_ni(k,ctr,stream); size_t m=n<16?n:16; for(size_t i=0;i<m;i++) out[i]=in[i]^stream[i]; in+=m; out+=m; n-=m; inc_be128(ctr); } }
static void ctr_crypt_ttable8(const aes128_sw_key *k,uint8_t ctr[16],const uint8_t *in,uint8_t *out,size_t n){
    const __m128i bswap=_mm_setr_epi8(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);
    while(n>=128){
        uint8_t ks[128], c[16];
        __m128i lo=_mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)ctr),bswap);
        uint64_t low=(uint64_t)_mm_cvtsi128_si64(lo);
        for(int i=0;i<8;i++){
            memcpy(c,ctr,16);
            uint64_t v=low+(uint64_t)i;
            for(int j=0;j<8;j++) c[15-j]=(uint8_t)(v>>(8*j));
            aes128_encrypt_ttable(k,c,ks+16*i);
        }
        __m256i ks0=_mm256_loadu_si256((const __m256i*)(ks));
        __m256i ks1=_mm256_loadu_si256((const __m256i*)(ks+32));
        __m256i ks2=_mm256_loadu_si256((const __m256i*)(ks+64));
        __m256i ks3=_mm256_loadu_si256((const __m256i*)(ks+96));
        _mm256_storeu_si256((__m256i*)(out),_mm256_loadu_si256((const __m256i*)(in))^ks0);
        _mm256_storeu_si256((__m256i*)(out+32),_mm256_loadu_si256((const __m256i*)(in+32))^ks1);
        _mm256_storeu_si256((__m256i*)(out+64),_mm256_loadu_si256((const __m256i*)(in+64))^ks2);
        _mm256_storeu_si256((__m256i*)(out+96),_mm256_loadu_si256((const __m256i*)(in+96))^ks3);
        for(int i=0;i<8;i++) inc_be128(ctr);
        in+=128; out+=128; n-=128;
    }
    ctr_crypt_ttable(k,ctr,in,out,n);
}
static void ctr_crypt_ni8(const aes128_ni_key *k,uint8_t ctr[16],const uint8_t *in,uint8_t *out,size_t n){
    const __m128i bswap=_mm_setr_epi8(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);
    while(n>=128){
        __m128i x[8], lo=_mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)ctr),bswap);
        uint64_t low=(uint64_t)_mm_cvtsi128_si64(lo);
        for(int i=0;i<8;i++){ uint8_t c[16]; memcpy(c,ctr,16); uint64_t v=low+(uint64_t)i; for(int j=0;j<8;j++) c[15-j]=(uint8_t)(v>>(8*j)); x[i]=aes128_encrypt_block_ni(k,_mm_loadu_si128((const __m128i*)c)); }
        __m256i ks0=_mm256_set_m128i(x[1],x[0]);
        __m256i ks1=_mm256_set_m128i(x[3],x[2]);
        __m256i ks2=_mm256_set_m128i(x[5],x[4]);
        __m256i ks3=_mm256_set_m128i(x[7],x[6]);
        _mm256_storeu_si256((__m256i*)(out),_mm256_loadu_si256((const __m256i*)(in))^ks0);
        _mm256_storeu_si256((__m256i*)(out+32),_mm256_loadu_si256((const __m256i*)(in+32))^ks1);
        _mm256_storeu_si256((__m256i*)(out+64),_mm256_loadu_si256((const __m256i*)(in+64))^ks2);
        _mm256_storeu_si256((__m256i*)(out+96),_mm256_loadu_si256((const __m256i*)(in+96))^ks3);
        for(int i=0;i<8;i++) inc_be128(ctr);
        in+=128; out+=128; n-=128;
    }
    while(n){ uint8_t stream[16]; aes128_encrypt_ni(k,ctr,stream); size_t m=n<16?n:16; for(size_t i=0;i<m;i++) out[i]=in[i]^stream[i]; in+=m; out+=m; n-=m; inc_be128(ctr); }
}

static void xor16(uint8_t a[16],const uint8_t b[16]){ for(int i=0;i<16;i++) a[i]^=b[i]; }
static void ghash_mulx(uint8_t x[16]){
    int lsb=x[15]&1;
    for(int j=15;j>0;j--) x[j]=(x[j]>>1)|((x[j-1]&1)<<7);
    x[0]>>=1;
    if(lsb) x[0]^=0xe1;
}
static void ghash_mulx4(uint8_t x[16]){ for(int i=0;i<4;i++) ghash_mulx(x); }
static void ghash_make_4bit_table(uint8_t tab[16][16],const uint8_t h[16]){
    memset(tab,0,16*16);
    for(int n=0;n<16;n++){
        uint8_t z[16]={0};
        for(int b=3;b>=0;b--){
            ghash_mulx(z);
            if((n>>b)&1) xor16(z,h);
        }
        memcpy(tab[n],z,16);
    }
}
static void ghash_mul_4bit(uint8_t x[16],const uint8_t tab[16][16]){
    uint8_t z[16]={0};
    for(int i=0;i<16;i++){
        ghash_mulx4(z); xor16(z,tab[x[i]>>4]);
        ghash_mulx4(z); xor16(z,tab[x[i]&15]);
    }
    memcpy(x,z,16);
}
static void ghash_update_4bit(uint8_t y[16],const uint8_t tab[16][16],const uint8_t *p,size_t n){ uint8_t b[16]; while(n){ memset(b,0,16); size_t m=n<16?n:16; memcpy(b,p,m); xor16(y,b); ghash_mul_4bit(y,tab); p+=m; n-=m; } }
static void put64be(uint8_t *p,uint64_t x){ for(int i=7;i>=0;i--){ p[i]=x; x>>=8; } }
static void gcm_encrypt(const aes128_ni_key *k,const uint8_t iv[12],const uint8_t *aad,size_t alen,const uint8_t *pt,uint8_t *ct,size_t n,uint8_t tag[16]){
    uint8_t h[16]={0}, y[16]={0}, j0[16]={0}, ctr[16], lenb[16], htab[16][16]; aes128_encrypt_ni(k,h,h); ghash_make_4bit_table(htab,h); memcpy(j0,iv,12); j0[15]=1; memcpy(ctr,j0,16); inc_be128(ctr);
    ctr_crypt_ni8(k,ctr,pt,ct,n); ghash_update_4bit(y,htab,aad,alen); ghash_update_4bit(y,htab,ct,n); memset(lenb,0,16); put64be(lenb,(uint64_t)alen*8); put64be(lenb+8,(uint64_t)n*8); xor16(y,lenb); ghash_mul_4bit(y,htab); aes128_encrypt_ni(k,j0,tag); xor16(tag,y);
}

static void gf_mul_x(uint8_t t[16]){ uint8_t carry=0; for(int i=0;i<16;i++){ uint8_t ncarry=t[i]>>7; t[i]=(uint8_t)((t[i]<<1)|carry); carry=ncarry; } if(carry) t[15]^=0x87; }
static void xts_crypt(const aes128_ni_key *data,const aes128_ni_key *tweak,const uint8_t iv[16],const uint8_t *in,uint8_t *out,size_t n,int decrypt){
    uint8_t tw[16]; aes128_encrypt_ni(tweak,iv,tw);
    for(size_t off=0;off+16<=n;off+=16){ uint8_t b[16]; for(int i=0;i<16;i++) b[i]=in[off+i]^tw[i]; __m128i x=_mm_loadu_si128((const __m128i*)b); x=decrypt?aes128_decrypt_block_ni(data,x):aes128_encrypt_block_ni(data,x); _mm_storeu_si128((__m128i*)b,x); for(int i=0;i<16;i++) out[off+i]=b[i]^tw[i]; gf_mul_x(tw); }
}

static int eqhex(const char *name,const uint8_t *a,const uint8_t *b,size_t n){ int ok=memcmp(a,b,n)==0; printf("%-24s %s\n",name,ok?"ok":"FAIL"); return ok; }
static double now(void){ return (double)clock()/CLOCKS_PER_SEC; }
static void bench(const char *name,void (*fn)(void*,size_t),void *ctx,size_t bytes){ double t0=now(); fn(ctx,bytes); double dt=now()-t0; printf("%-24s %8.2f MiB/s\n",name,(bytes/(1024.0*1024.0))/dt); }

typedef struct { aes128_sw_key sw; aes128_ni_key ni,tw; uint8_t *buf,*out; } bench_ctx;
static void b_ref(void *v,size_t n){ bench_ctx*c=v; uint8_t ctr[16]={0}; ctr_crypt_ref(&c->sw,ctr,c->buf,c->out,n); }
static void b_ttable(void *v,size_t n){ bench_ctx*c=v; uint8_t ctr[16]={0}; ctr_crypt_ttable(&c->sw,ctr,c->buf,c->out,n); }
static void b_ttable8(void *v,size_t n){ bench_ctx*c=v; uint8_t ctr[16]={0}; ctr_crypt_ttable8(&c->sw,ctr,c->buf,c->out,n); }
static void b_t2(void *v,size_t n){ bench_ctx*c=v; uint8_t ctr[16]={0}; ctr_crypt_t2(&c->sw,ctr,c->buf,c->out,n); }
static void b_ni1(void *v,size_t n){ bench_ctx*c=v; uint8_t ctr[16]={0}; ctr_crypt_ni1(&c->ni,ctr,c->buf,c->out,n); }
static void b_ni8(void *v,size_t n){ bench_ctx*c=v; uint8_t ctr[16]={0}; ctr_crypt_ni8(&c->ni,ctr,c->buf,c->out,n); }
static void b_gcm(void *v,size_t n){ bench_ctx*c=v; uint8_t iv[12]={0},tag[16]; gcm_encrypt(&c->ni,iv,NULL,0,c->buf,c->out,n,tag); }
static void b_xts(void *v,size_t n){ bench_ctx*c=v; uint8_t iv[16]={0}; xts_crypt(&c->ni,&c->tw,iv,c->buf,c->out,n&~(size_t)15,0); }

static int load_file(const char *path,uint8_t **buf,size_t *n){
    FILE *fp=fopen(path,"rb");
    if(!fp) return 0;
    if(fseek(fp,0,SEEK_END)!=0){ fclose(fp); return 0; }
    long len=ftell(fp);
    if(len<=0){ fclose(fp); return 0; }
    rewind(fp);
    *buf=(uint8_t*)malloc((size_t)len);
    if(!*buf){ fclose(fp); return 0; }
    *n=fread(*buf,1,(size_t)len,fp);
    fclose(fp);
    return *n==(size_t)len;
}

static void fill_default(uint8_t *buf,size_t n){ for(size_t i=0;i<n;i++) buf[i]=(uint8_t)i; }

int main(int argc,char **argv){
    aes_tables_init();
    const uint8_t key[16]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    const uint8_t pt[16]={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
    const uint8_t ct_exp[16]={0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a};
    aes128_sw_key sw; aes128_ni_key ni; uint8_t a[16],b[16],c[16],d[16]; aes128_key_expand_sw(&sw,key); aes128_key_expand_ni(&ni,key);
    aes128_encrypt_ref(&sw,pt,a); aes128_encrypt_ttable(&sw,pt,b); aes128_encrypt_ni(&ni,pt,c); aes128_encrypt_t2(&sw,pt,d);
    int ok=1; ok&=eqhex("AES reference",a,ct_exp,16); ok&=eqhex("AES 4-table",b,ct_exp,16); ok&=eqhex("AES T2-table",d,ct_exp,16); ok&=eqhex("AES-NI",c,ct_exp,16);
    uint8_t ctr[16]={0}, m[256]={0}, x[256], y[256]; ctr_crypt_ref(&sw,ctr,m,x,256); memset(ctr,0,16); ctr_crypt_ni8(&ni,ctr,m,y,256); ok&=eqhex("CTR ref vs AES-NI",x,y,256);
    memset(ctr,0,16); ctr_crypt_ttable8(&sw,ctr,m,y,256); ok&=eqhex("CTR ref vs Ttable8",x,y,256);
    const uint8_t gkey[16]={0}, giv[12]={0}; const uint8_t gtag_exp[16]={0x58,0xe2,0xfc,0xce,0xfa,0x7e,0x30,0x61,0x36,0x7f,0x1d,0x57,0xa4,0xe7,0x45,0x5a}; uint8_t tag[16]; aes128_ni_key gni; aes128_key_expand_ni(&gni,gkey); gcm_encrypt(&gni,giv,NULL,0,NULL,NULL,0,tag); ok&=eqhex("GCM empty vector",tag,gtag_exp,16);
    uint8_t xts_key2[16]; memset(xts_key2,0x55,16); aes128_ni_key tw; aes128_key_expand_ni(&tw,xts_key2); uint8_t iv[16]={1}, xp[32],xc[32],xr[32]; for(int i=0;i<32;i++) xp[i]=(uint8_t)i; xts_crypt(&ni,&tw,iv,xp,xc,32,0); xts_crypt(&ni,&tw,iv,xc,xr,32,1); ok&=eqhex("XTS round trip",xp,xr,32);
    uint8_t *buf=NULL,*out=NULL;
    size_t n=8*1024*1024;
    const char *bench_file=NULL;
    if(argc==3 && strcmp(argv[1],"--bench-file")==0) bench_file=argv[2];
    if(bench_file && load_file(bench_file,&buf,&n)) printf("\nBenchmark file: %s (%zu bytes)\n",bench_file,n);
    else {
        if(bench_file) fprintf(stderr,"warning: cannot read %s, using generated buffer\n",bench_file);
        buf=(uint8_t*)malloc(n);
        fill_default(buf,n);
        puts("\nBenchmark buffer: generated 8 MiB pattern");
    }
    out=(uint8_t*)malloc(n);
    if(!buf||!out){ fprintf(stderr,"allocation failed\n"); free(buf); free(out); return 1; }
    bench_ctx bc={sw,ni,tw,buf,out};
    puts("Benchmark");
    bench("CTR reference",&b_ref,&bc,n);
    bench("CTR 4-table",&b_ttable,&bc,n);
    bench("CTR 4-table+shuffle+AVX2",&b_ttable8,&bc,n);
    bench("CTR T2-table",&b_t2,&bc,n);
    bench("CTR AES-NI single",&b_ni1,&bc,n);
    bench("CTR AES-NI+shuffle+AVX2",&b_ni8,&bc,n);
    bench("GCM CTR+AES-NI+GHASH",&b_gcm,&bc,n);
    bench("XTS AES-NI+tweak",&b_xts,&bc,n);
    free(buf); free(out);
    return ok?0:1;
}
