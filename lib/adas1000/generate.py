#!/usr/bin/env python3

import re
from collections import OrderedDict
from typing import List, Dict

from PySide2.QtCore import QXmlStreamReader, QIODevice


def identifierify(s):
	return s.replace("-", "_")


class XmlNode:
	def __init_subclass__(cls, name = None, **kwargs):
		cls.XML_NAME = name

	def handle_element(self, xsr, **attributes):
		handler_name = f'handle_element_{identifierify(xsr.name())}'

		if hasattr(self, handler_name):
			# print(f"xsr - {handler_name}()")
			getattr(self, handler_name)(xsr, **attributes)
		else:
			# print(f"unhandled element: {xsr.name()}")
			xsr.skipCurrentElement()

	def handle_characters(self, xsr, text):
		pass

	@classmethod
	def attributes_to_dict(cls, xsr):
		attributes = {}

		for attribute in filter(lambda a: a.namespaceUri() == '', xsr.attributes()):
			attributes[identifierify(attribute.name())] = attribute.value()

		return attributes

	@classmethod
	def from_xml(cls, xsr):
		# print(f"xsr - {xsr.name()}")
		self = cls(**cls.attributes_to_dict(xsr))

		# while xsr.readNext():
		# 	tt = xsr.tokenType()

		# 	if tt == QXmlStreamReader.StartElement:
		# 		self.handle_element(xsr, **cls.attributes_to_dict(xsr))
		# 	elif tt == QXmlStreamReader.Characters:
		# 		self.handle_characters(xsr, xsr.text())
		# 	elif tt == QXmlStreamReader.EndElement:
		# 		break

		while xsr.readNextStartElement():
			# print(f"xsr - el: {xsr.name()}")
			self.handle_element(xsr, **cls.attributes_to_dict(xsr))
			# xsr.skipCurrentElement()
		# print(f"xsr{self.__class__.__name__}")

		return self


class Documentable(XmlNode):
	doc: str

	def __init__(self, **kwargs):
		self.doc = ""
		super().__init__(**kwargs)

	def handle_element_doc(self, xsr):
		self.doc = xsr.readElementText()


class Item(Documentable):
	name: str

	def __init__(self, name, **kwargs):
		super().__init__(**kwargs)
		self.name = name


class ItemWithBase(Item):
	base: str

	def __init__(self, base = None, **kwargs):
		super().__init__(**kwargs)
		self.base = base


INT_BASES = sorted({
	'0b': 2,
	'0o': 8,
	'0x': 16,
}.items(), key = lambda base: len(base[0]), reverse = True)


def parse_int(s):
	s, base = next(map(
		lambda base: (s[len(base[0]):], base[1]),
		filter(lambda base: s.startswith(base[0]), INT_BASES)
	), (s, 10))

	return int(s, base)


class Range:
	min: int
	max: int

	def __init__(self, min, max = None):
		if max is None:
			max = min
		if max < min:
			min, max = max, min

		self.min = min
		self.max = max if max is not None else min

	def __repr__(self):
		return f"{type(self).__name__}({self.min}, {self.max})"

	def __str__(self):
		if self.min == self.max:
			return f"{self.min}"
		else:
			return f"{self.max}:{self.min}"

	RE = re.compile(r'^(\d+)(?::(\d+))?$')

	@classmethod
	def parse(cls, s):
		if cls.RE.match(s):
			a, b = cls.RE.search(s).groups()

			if a is not None:
				a = parse_int(a)
			if b is not None:
				b = parse_int(b)

			return cls(a, b)
		else:
			raise RuntimeError(f"invalid range {s!r}")


class Variant(Item, name="variant"):
	value: int

	def __init__(self, value, **kwargs):
		super().__init__(**kwargs)
		self.value = parse_int(value)


class Enum(XmlNode, name="enum"):
	variants: List[Variant]

	def __init__(self, **kwargs):
		super().__init__(**kwargs)
		self.variants = []

	def handle_element_variant(self, xsr, **attrs):
		self.variants.append(Variant.from_xml(xsr))


class Field(ItemWithBase, name="field"):
	range: Range
	enum: Enum

	def __init__(self, range, **kwargs):
		super().__init__(**kwargs)
		self.range = Range.parse(range)
		self.enum = None

	def handle_element_enum(self, xsr):
		self.enum = Enum.from_xml(xsr)


class Register(ItemWithBase, name="register"):
	address: int
	reset_value: int
	fields: List[Field]

	def __init__(self, address, reset_value, **kwargs):
		super().__init__(**kwargs)
		self.address = parse_int(address)
		self.reset_value = parse_int(reset_value)
		self.fields = []

	def __getitem__(self, name) -> Field:
		return next(filter(lambda f: f.name == name, self.fields))

	def handle_element_field(self, xsr, **attrs):
		field = Field.from_xml(xsr)
		if field.base:
			base = self[field.base]

			if field.enum is None:
				field.enum = base.enum
		self.fields.append(field)


class Device(Item, name="device"):
	name: str
	registers: Dict[str, Register]

	def __init__(self, **kwargs):
		super().__init__(**kwargs)
		self.registers = OrderedDict()

	def handle_element_register(self, xsr, name, **attrs):
		self.registers[name] = Register.from_xml(xsr)


class FileIoDevice(QIODevice):
	def __init__(self, file):
		super().__init__()
		self._file = file

		mode = 0
		if 'r' in self._file.mode:
			mode |= QIODevice.ReadOnly
		if 'w' in self._file.mode:
			mode |= QIODevice.WriteOnly
		self.open(mode)

	def __enter__(self):
		return self

	def __exit__(self, Exc, exc, tb):
		if not self._file.closed:
			self._file.close()

		if exc:
			raise exc

	def readData(self, max_len):
		return self._file.read(max_len)


if __name__ == '__main__':
	dev = None

	with FileIoDevice(open("adas1000.xml", 'rb')) as f:
		xsr = QXmlStreamReader(f)
		if xsr.readNextStartElement():
			dev = Device.from_xml(xsr)

	def n(s):
		return re.sub(r"[^a-zA-Z0-9]+", "_", s).upper()

	if dev:
		print("#ifndef ADAS1000_REG_H")
		print("#define ADAS1000_REG_H")

		for _, reg in dev.registers.items():
			reg_name = f"{n(dev.name)}_{n(reg.name)}"
			print(f"#define {reg_name:<45} 0x{reg.address:02x}")

			for fld in reg.fields:
				fld_len = fld.range.max - fld.range.min + 1

				mask = ((1 << fld_len) - 1) << fld.range.min
				mask_name = f"{n(dev.name)}_{n(reg.name)}_{n(fld.name)}"

				parts = [
					"#define",
					f"{mask_name:<45}",
					f"0b{mask:024b}",
				]

				# if fld.doc:
				# 	parts.append(f"// {fld.doc}")

				print(" ".join(parts))

				if fld.enum:
					for var in fld.enum.variants:
						var_name = f"{n(dev.name)}_{n(reg.name)}_{n(fld.name)}_{n(var.name)}"

						parts = [
							"#define",
							f"{var_name:<45}",
							f"0b{var.value << fld.range.min:024b}",
						]

						# if var.doc:
						# 	parts.append(f"// {var.doc}")

						print(" ".join(parts))

		print("#endif")
