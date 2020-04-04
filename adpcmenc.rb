# coding: utf-8

scale = 16

def nextscale scale, coeff
	scale * [233, 233, 233, 233, 310, 375, 453, 549][coeff] / 256
end

until $stdin.eof?
	ss = $stdin.read(16).unpack("s>*")
	ss = ss + [0] * (8 - ss.size)

	coeffs = ss.map{|s| (s.to_f / scale).round.clamp(-7, 7) }
	ns = nextscale scale, coeffs.map(&:abs).max
	ns < 16 and coeffs[7] = coeffs[7] < 0 ? -4 : 4
	32768/7 < ns and coeffs.map! { |c| c.clamp(-3, 3) }

	#ss = coeffs.map{|c| (scale * c).clamp(-32768, 32767) }
	scale = nextscale scale, coeffs.map(&:abs).max

	$stdout.write coeffs.each_slice(2).map{|a,b| ((a & 15) << 4) | (b & 15) }.pack("C*")
end
