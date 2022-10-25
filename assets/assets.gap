# Test GAP file

export,name=beekind,filename=beekind.gbin,type=gbin,format=binary
export,name=beekind,filename="../src/generated/game_gbin.h",type=gbin,format=c_array
export,name=beekind,filename="../src/generated/game_assets.h",type=definitions,format=cheader

#==============================================================================
#
#	SPRITESHEET
#
#==============================================================================

loadimage,src="/images/tilemap_packed.png"

imagegroup,group=1,name="SPRITES"
imagearray,width=16,height=16,xcount=10,ycount=6,format=ARGB1555

imagegroup,group=2,name="SPRITES_FLIP"
imagearray,width=16,height=16,xcount=10,ycount=6,hflip=1,format=ARGB1555
