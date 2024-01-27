/*****************************
 *
 * ngramchar User Defined Functions
 *
 * Copyright DingQiang Liu(dingqiangliu@gmail.com), 2012 - 2017
 */

-- Step 1: Create LIBRARY 
\set libfile '\''`pwd`'/.libs/ngramchar.so\'';
CREATE LIBRARY ngramchar AS :libfile;

-- Step 2: Create cube/rollup Factory
\set tmpfile '/tmp/ngramcharinstall.sql'
\! cat /dev/null > :tmpfile

\t
\o :tmpfile
select 'CREATE TRANSFORM FUNCTION '||replace(obj_name, 'Factory', '')||' AS LANGUAGE ''C++'' NAME '''||obj_name||''' LIBRARY ngramchar /*not*/ fenced;' from user_library_manifest where lib_name='ngramchar' and obj_type='Transform Function';
select 'GRANT EXECUTE ON TRANSFORM FUNCTION '||replace(obj_name, 'Factory', '')||' () to PUBLIC;' from user_library_manifest where lib_name='ngramchar' and obj_type='Transform Function';
select 'CREATE FUNCTION '||replace(obj_name, 'Factory', '')||' AS LANGUAGE ''C++'' NAME '''||obj_name||''' LIBRARY ngramchar /*not*/ fenced;' from user_library_manifest where lib_name='ngramchar' and obj_type='Scalar Function';
select 'GRANT EXECUTE ON FUNCTION '||replace(obj_name, 'Factory', '')||' ('||arg_types||') to PUBLIC;' from user_library_manifest where lib_name='ngramchar' and obj_type='Scalar Function';

\o
\t

\i :tmpfile
\! rm -f :tmpfile
