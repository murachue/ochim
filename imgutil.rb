#!/usr/bin/env ruby

def main opname, outfn=nil
	result = nil

	case opname
	when "ia88toia44"
		result = $stdin.read.bytes.each_slice(2).map{|i,a| (i & 0xF0) | (a >> 4) }.pack("C*")
	when "rxxa8888toia44" # for imagemagick < 7 that does not support graya
		result = $stdin.read.bytes.each_slice(4).map{|r,g,b,a| (r & 0xF0) | (a >> 4) }.pack("C*")
	when "rgb888torgba5551be"
		result = $stdin.read.bytes.each_slice(3).map{|r,g,b| ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | 1 }.pack("n*")
	when "rgba8888torgba5551be"
		result = $stdin.read.bytes.each_slice(4).map{|r,g,b,a| ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | ((a >> 7) << 0) }.pack("n*")
	when "640x480i_swap_interleave"
		width = 640
		pixels = $stdin.read.unpack("N*")
		fields = pixels.each_slice(width * 16 / 32).each_slice(2).to_a.transpose # "un-interleave" by fields
		result = fields.flat_map{ |field_lines|
			lines_per_blit = 3
			field_lines.each_slice(lines_per_blit).flat_map{ |lines|
				# swap a pair of 32bit (2-pixels on 16bpp) on odd-line(s): RDP TMEM spec, for load_block with dxt=0 (fastest on 640px wide)
				lines.each_with_index.flat_map { |line, i|
					i % 2 == 0 and next line
					line.each_slice(2).flat_map(&:reverse)
				}
			}
		}.pack("N*")
	else; raise "?op #{opname}"
	end

	if outfn
		File.binwrite outfn, result
	else
		$stdout.binmode.write result
	end
end

$0 == __FILE__ and main *ARGV
