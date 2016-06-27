! Copyright (C) 2011  ARP1-SIM <urpsim@smr.arpa.emr.it>
!
! This program is free software; you can redistribute it and/or modify
! it under the terms of the GNU General Public License as published by
! the Free Software Foundation; either version 2 of the License.
!
! This program is distributed in the hope that it will be useful,
! but WITHOUT ANY WARRANTY; without even the implied warranty of
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
! GNU General Public License for more details.
!
! You should have received a copy of the GNU General Public License
! along with this program; if not, write to the Free Software
! Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
!
! Author: Paolo Patruno <ppatruno@arpa.emr.it>

program dump_dballe
USE,INTRINSIC :: iso_c_binding
include "dballeff.h"

! ****************************************************
! * Dump the contents of a weather messages in a file
! ****************************************************

!implicit none

integer :: handle, nstaz, ndata, nattr
integer :: i, i1, i2, type1, l1, type2, l2
integer ::  height, year, month, day, hour, minute, second
character(len=255) :: fname,prettyvalue
character(len=20)  :: cname, rep_memo,value, avalue
character(len=10) :: btable, starbtable
doubleprecision ::dlat,dlon

external errorrep
call getarg(1,fname)

ierr = idba_error_set_callback(0, C_FUNLOC(errorrep), 0, i)

!     Open a session
ierr = idba_messaggi(handle, fname, "r", "BUFR")

!     Query all the stations
do while (.true.)
  ierr = idba_quantesono(handle, nstaz)
  if (nstaz .eq. DBA_MVI) exit

  ierr = idba_elencamele(handle)
  ierr = idba_enq(handle, "name", cname)
  ierr = idba_enq(handle, "lat", dlat)
  ierr = idba_enq(handle, "lon", dlon)
  ierr = idba_enq(handle, "height", height)
  ierr = idba_enq(handle,"rep_memo",rep_memo)

  write (*,*) "Staz: ",trim(cname)," (",dlat,",",dlon,")"," h:",height," network: ",rep_memo
  !ierr = idba_set(handle,"varlist","B12101,B11002") ! only on DB is valid

  ierr = idba_voglioquesto(handle,ndata)
  !write (*,*) " ",ndata," dati:"                  ! only on DB is valid
  do i1=1, ndata
    ierr = idba_dammelo(handle,btable)
    if (btable /= "B12101" .and. btable /= "B11002") cycle 

    ierr = idba_enqdate(handle, year, month, day, hour, minute, second)
    ierr = idba_enqlevel(handle, type1, l1, type2, l2)
    ierr = idba_enq(handle,btable,value)

    print*,"----"
    write (*,*) "date time: ",year, month, day, hour, minute
    ierr = idba_spiegal(handle,type1,l1,type2,l2,prettyvalue)
    write (*,*) trim(prettyvalue)
    ierr = idba_spiegab(handle,btable,value,prettyvalue)
    write (*,*) trim(prettyvalue)
    
    ierr = idba_voglioancora (handle,nattr)
    if (nattr > 0) then
      write (*,*) "   ",nattr," attributi:"
      do i2=1, nattr
        ierr = idba_ancora(handle,starbtable)
        ierr = idba_enq(handle,starbtable,avalue)
        write(*,*) "    attr ",trim(starbtable),": ",avalue
      enddo
    end if
  enddo
enddo

ierr = idba_fatto(handle)
      
call exit (0)

end program dump_dballe

SUBROUTINE errorrep(val) BIND(C)
USE,INTRINSIC :: iso_c_binding
include "dballeff.h"
integer :: val
character(len=1000) :: buf

ier = idba_error_code()
if (ier.ne.0) then
  print *,ier," error in ",val
  call idba_error_message(buf)
  print *,trim(buf)
  call idba_error_context(buf)
  print *,trim(buf)
  call idba_error_details(buf)
  print *,trim(buf)
  call exit (1)
end if
return

end subroutine errorrep
