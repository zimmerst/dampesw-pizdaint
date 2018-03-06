#/bin/bash
patchdir=${1}
swdir=${2}
echo "will patch ${swdir} with content from ${patchdir}"
for f in $(find ${patchdir} -type f);
do
 infile=${f}
 outfile=${infile/${patchdir}/${swdir}}
 #echo "${infile} -> ${outfile}"
 cp -v ${infile} ${outfile}
done
