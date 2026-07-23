#!/usr/bin/env python3
"""ECDSA existential forgery when an application accepts an attacker-chosen digest.

This is an educational secp256k1 implementation. It demonstrates an API/protocol
misuse, not a break of ECDSA and not a forgery for an attacker-chosen message.
"""

from dataclasses import dataclass
from hashlib import sha256


P_FIELD = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F
N = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141
GX = 0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798
GY = 0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8


@dataclass(frozen=True)
class Point:
    x: int
    y: int


G = Point(GX, GY)
INF = None


def inverse(a: int, modulus: int) -> int:
    return pow(a % modulus, -1, modulus)


def point_add(p, q):
    if p is INF:
        return q
    if q is INF:
        return p
    if p.x == q.x and (p.y + q.y) % P_FIELD == 0:
        return INF
    if p == q:
        slope = (3 * p.x * p.x) * inverse(2 * p.y, P_FIELD) % P_FIELD
    else:
        slope = (q.y - p.y) * inverse(q.x - p.x, P_FIELD) % P_FIELD
    x = (slope * slope - p.x - q.x) % P_FIELD
    y = (slope * (p.x - x) - p.y) % P_FIELD
    return Point(x, y)


def scalar_mul(k: int, p: Point):
    result = INF
    addend = p
    k %= N
    while k:
        if k & 1:
            result = point_add(result, addend)
        addend = point_add(addend, addend)
        k >>= 1
    return result


def digest(message: bytes) -> int:
    return int.from_bytes(sha256(message).digest(), "big") % N


def sign_digest(e: int, private_key: int, nonce: int):
    r_point = scalar_mul(nonce, G)
    r = r_point.x % N
    s = inverse(nonce, N) * (e + private_key * r) % N
    if r == 0 or s == 0:
        raise ValueError("invalid nonce; choose another")
    return r, s


def verify_digest(e: int, signature, public_key: Point) -> bool:
    r, s = signature
    if not (1 <= r < N and 1 <= s < N):
        return False
    w = inverse(s, N)
    check = point_add(scalar_mul(e * w, G), scalar_mul(r * w, public_key))
    return check is not INF and check.x % N == r


def forge_for_chosen_digest(public_key: Point, u: int, v: int):
    """Construct (e', r', s') such that Verify(P, e', (r',s')) succeeds."""
    r_point = point_add(scalar_mul(u, G), scalar_mul(v, public_key))
    r = r_point.x % N
    if r == 0:
        raise ValueError("unlucky u,v: r is zero")
    v_inv = inverse(v, N)
    s = r * v_inv % N
    e = r * u * v_inv % N
    return e, (r, s), r_point


def short(x: int) -> str:
    h = f"{x:064x}"
    return h[:16] + "..." + h[-16:]


def main():
    # Fixed values make the experiment reproducible; never use fixed nonces in production.
    private_key = int.from_bytes(sha256(b"crypto-lab-private-key").digest(), "big") % N
    public_key = scalar_mul(private_key, G)
    message = b"Authorize payment of 1 BTC"
    e_real = digest(message)
    nonce = int.from_bytes(sha256(b"demo-only-fixed-nonce").digest(), "big") % N
    legitimate = sign_digest(e_real, private_key, nonce)

    # The attacker needs only P. They choose u,v, then derive a digest and signature together.
    u = int.from_bytes(sha256(b"attacker-u").digest(), "big") % N or 1
    v = int.from_bytes(sha256(b"attacker-v").digest(), "big") % N or 1
    e_forged, forged, r_point = forge_for_chosen_digest(public_key, u, v)

    legit_ok = verify_digest(e_real, legitimate, public_key)
    forged_digest_ok = verify_digest(e_forged, forged, public_key)
    forged_message_ok = verify_digest(e_real, forged, public_key)
    equation_ok = point_add(scalar_mul(e_forged * inverse(forged[1], N), G),
                            scalar_mul(forged[0] * inverse(forged[1], N), public_key)) == r_point

    print("=== secp256k1 ECDSA digest-forgery experiment ===")
    print("public key x       =", short(public_key.x))
    print("public key y       =", short(public_key.y))
    print("SHA256(message)    =", short(e_real))
    print("legitimate r       =", short(legitimate[0]))
    print("legitimate s       =", short(legitimate[1]))
    print("legitimate verify  =", "PASS" if legit_ok else "FAIL")
    print()
    print("attacker knows private key? NO")
    print("chosen u           =", short(u))
    print("chosen v           =", short(v))
    print("R' = uG + vP, x'   =", short(r_point.x))
    print("forged digest e'   =", short(e_forged))
    print("forged r'          =", short(forged[0]))
    print("forged s'          =", short(forged[1]))
    print("verification equation R'=uG+vP =", "PASS" if equation_ok else "FAIL")
    print("verify chosen digest e'         =", "PASS" if forged_digest_ok else "FAIL")
    print("e' == SHA256(target message)?   =", e_forged == e_real)
    print("verify target message hash      =", "PASS" if forged_message_ok else "FAIL (expected)")
    print()
    print("CONCLUSION:")
    print("PASS: a valid (digest, signature) pair was created without the private key.")
    print("PASS: it does NOT sign the chosen target message; ECDSA itself is not broken.")

    assert legit_ok and forged_digest_ok and equation_ok
    assert e_forged != e_real and not forged_message_ok


if __name__ == "__main__":
    main()
