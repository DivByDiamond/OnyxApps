local module = require("smoke_module")
assert(module.answer == 42)
assert(math.sqrt(81) == 9)
assert(math.floor(3.75) == 3)
assert(tonumber("3.25") == 3.25)
local sum = 0
for i = 1, 10 do sum = sum + i end
assert(sum == 55)
local values = {}
for i = 1, 200 do values[i] = {i, tostring(i)} end
assert(values[100][1] == 100)
values = nil
collectgarbage("collect")
local ok, message = pcall(error, "caught-smoke-error")
assert(not ok and message:find("caught%-smoke%-error"))
local chunk, syntax = load("local =")
assert(chunk == nil and type(syntax) == "string")
print("lua-smoke-ok", module.answer, math.floor(math.sqrt(81)))
