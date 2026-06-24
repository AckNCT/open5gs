# Copyright (C) 2026 by the M-Time Simulation project
#
# This file is part of Open5GS (AckNCT/open5gs `mtime-lcs` fork).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# LCS-AP (SLs interface, 3GPP TS 29.171) library.
# Increment 1: scaffolding only. The asn1c-generated codec dependency
# (liblcsap_asn1c_dep, mirroring libasn1c_s1ap_dep) is added in increment 2.

liblcsap_sources = files('''
    ogs-lcs-ap.h

    ogs-lcs-ap.c
'''.split())

liblcsap_inc = include_directories('.')

liblcsap = library('ogslcsap',
    sources : liblcsap_sources,
    version : libogslib_version,
    c_args : '-DOGS_LCS_AP_COMPILATION',
    include_directories : [liblcsap_inc, libinc],
    dependencies : libcore_dep,
    install : true)

liblcsap_dep = declare_dependency(
    link_with : liblcsap,
    include_directories : [liblcsap_inc, libinc],
    dependencies : libcore_dep)
