--[[
LuCI - Configuration

Description:
Some LuCI configuration values read from uci file "luci"
读取UCI格式的luci配置


FileId:
$Id$

License:
Copyright 2008 Steven Barth <steven@midlink.org>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at 

	http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

]]--

local util = require "luci.util"

--[[ 创建luci配置模块
--   该模块table设置了一个携带__index元方法的元表
--   当访问该模块中的luci配置参数key时都会触发__index元方法
]]--
module("luci.config",
		function(m)
			if pcall(require, "luci.model.uci") then
				local config = util.threadlocal()
				setmetatable(m, {
					__index = function(tbl, key)
                        -- 如果config中已经存在对应的参数值,则直接就可以返回；否则需要从相应的UCI文件中获取
						if not config[key] then
							config[key] = luci.model.uci.cursor():get_all("luci", key)
						end
						return config[key]
					end
				})
			end
		end)