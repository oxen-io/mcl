from montgomery import *

g_W = 52 # use 52-bit integer multiplication
g_N = 8 # store 381-bit integer in 8 bytes N arrays
g_vN = 2 # loop unroll

# return w-bit array of size N
def toArray(x, W=g_W, N=g_N):
  mask = getMask(W)
  a=[]
  for i in range(N):
    a.append(x & mask)
    x >>= W
  return a


class BLS12:
  def __init__(self, z=-0xd201000000010000):
    self.M = 1<<256
    self.H = 1<<128
    self.z = z
    self.L = self.z**2 - 1
    self.r = self.L*(self.L+1) + 1
    self.p = (z-1)**2*self.r//3 + z

def expand(name, v):
  if type(v) == int:
    s = f'{hex(v)}, '*8
  elif type(v) == list:
    s = ', '.join(map(hex, v)) + ' '
  print(f'static const CYBOZU_ALIGN(64) uint64_t {name}_[] = {{ {s}}};')

def expandN(name, v, n=1):
  if n > 1:
    name = f'{name}A'
  print(f'static const CYBOZU_ALIGN(64) uint64_t {name}_[] = {{')
  for i in range(len(v)):
    print(('\t' + f'{hex(v[i])}, '*n*8).strip())
  print('};')

def expandN3(name, vx, vy, vz, n=1):
  if n > 1:
    name = f'{name}A'
  print(f'static const CYBOZU_ALIGN(64) uint64_t {name}_[] = {{')
  for i in range(len(vx)):
    print(('\t' + f'{hex(vx[i])}, '*n*8).strip())
  for i in range(len(vy)):
    print(('\t' + f'{hex(vy[i])}, '*n*8).strip())
  for i in range(len(vz)):
    print(('\t' + f'{hex(vz[i])}, '*n*8).strip())
  print('};')

def putCode(curve, mont):
  p = curve.p
  rw = pow(-3, (p+1)//4, p)
  rw = p-(rw+1)//2
  if (rw*rw+rw+1)%p != 0:
    print(f'ERR rw {rw=}')
    return

  print('// generated by src/gen_msm_para.py')
  print(f'static const uint64_t g_mask = {hex(mont.mask)};')
  # for G
  expand('g_mask', mont.mask)
  expand('g_rp', mont.rp)
  expandN('g_ap', toArray(curve.p)) # array of p

  m64to52 = toArray(mont.toMont(2**32))
  m52to64 = toArray(mont.toMont(pow(2**32, -1, curve.p)))
  expand('g_m64to52u', m64to52)
  expand('g_m52to64u', m52to64)

  # for FpM/FpMA
  expand('g_offset', [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])
  for n in [1, 2]:
    expandN('g_zero', toArray(0), n) # FpM::zero()
    expandN('g_R', toArray(mont.R), n) # FpM::one()
    expandN('g_R2', toArray(mont.R2), n) # FpM::R2()
    expandN('g_rawOne', toArray(1), n) # FpM::rawOne()
    expandN('g_m64to52', m64to52, n)
    expandN('g_m52to64', m52to64, n)
    expandN('g_rw', toArray(mont.toMont(rw)), n)
    # for EcM/EcMA
    b = 4
    expandN('g_b3', toArray(mont.toMont(b*3)), n)
    expandN3('g_zeroJacobi', toArray(0), toArray(0), toArray(0), n)
    expandN3('g_zeroProj', toArray(0), toArray(1), toArray(0), n)

  print(f'''
struct G {{
	static const Vec& mask() {{ return *(const Vec*)g_mask_; }}
	static const Vec& rp() {{ return *(const Vec*)g_rp_; }}
	static const Vec* ap() {{ return (const Vec*)g_ap_; }}
}};
''')

def main():
  curve = BLS12()

  mont = Montgomery(curve.p)
#  print('#if 0')
#  mont.put()
#  print('#endif')
  putCode(curve, mont)

if __name__ == '__main__':
  main()

