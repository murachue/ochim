# coding: utf-8

scale = 16

def nextscale scale, coeff
	scale * [233, 233, 233, 233, 310, 375, 453, 549][coeff] / 256
end

until $stdin.eof?
	coeffs = $stdin.read(4).unpack("C*").flat_map{|c2| [c2, c2 << 4] }.pack("C*").unpack("c*").map{|c| c >> 4 }
	ss = coeffs.map{|c| (scale * c).clamp(-32768, 32767) }
	scale = nextscale scale, coeffs.map(&:abs).max

	$stdout.write ss.pack("s>*")
end
