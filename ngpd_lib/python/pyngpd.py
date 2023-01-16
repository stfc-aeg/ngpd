from  pyngpd_cffi import ffi, lib

class pyngpd:
	def __init__(self, nCards, dummy=0):
		self.nCard = nCards
		self.path = lib.ngpd_config_ngzmp(nCards, ffi.NULL, -1, ffi.NULL, -1, 2, 0, 1, dummy)

	def 