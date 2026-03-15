from openpilot.common.params_pyx import Params as _ParamsBase, ParamKeyFlag, ParamKeyType, UnknownKeyName
assert _ParamsBase
assert ParamKeyFlag
assert ParamKeyType
assert UnknownKeyName


class Params(_ParamsBase):
  """Extended Params with get_int/get_float/put_int helpers for CarrotPilot compatibility."""

  def get_int(self, key, default=0):
    val = self.get(key)
    if val is None:
      return default
    try:
      return int(val)
    except (ValueError, TypeError):
      return default

  def get_float(self, key, default=0.0):
    val = self.get(key)
    if val is None:
      return default
    try:
      return float(val)
    except (ValueError, TypeError):
      return default

  def put_int(self, key, value):
    self.put(key, int(value))

  def put_int_nonblocking(self, key, value):
    self.put_nonblocking(key, int(value))

  def put_float(self, key, value):
    self.put(key, float(value))

  def put_float_nonblocking(self, key, value):
    self.put_nonblocking(key, float(value))

if __name__ == "__main__":
  import sys

  params = Params()
  key = sys.argv[1]
  assert params.check_key(key), f"unknown param: {key}"

  if len(sys.argv) == 3:
    val = sys.argv[2]
    print(f"SET: {key} = {val}")
    params.put(key, val)
  elif len(sys.argv) == 2:
    print(f"GET: {key} = {params.get(key)}")
