#!/usr/bin/env python3

# vim: ai:ts=4:sw=4

import sys
from os import listdir
import os
import re
import yaml
import pprint
import argparse

from devicetree import parse_file

# globals
compatibles = {}
phandles = {}
aliases = {}
chosen = {}
reduced = {}

def convert_string_to_label(s):
  # Transmute ,- to _
  s = s.replace("-", "_");
  s = s.replace(",", "_");
  return s

def get_all_compatibles(d, name, comp_dict):
  if 'props' in d:
    compat = d['props'].get('compatible')
    enabled = d['props'].get('status')

  if enabled == "disabled":
    return comp_dict

  if compat != None:
    comp_dict[name] = compat

  if name != '/':
    name += '/'

  if isinstance(d,dict):
    if d['children']:
      for k,v in d['children'].items():
        get_all_compatibles(v, name + k, comp_dict)

  return comp_dict

def get_aliases(root):
  if 'children' in root:
    if 'aliases' in root['children']:
      for k,v in root['children']['aliases']['props'].items():
        aliases[v] = k

  return

def get_compat(node):

  compat = None

  if 'props' in node:
    compat = node['props'].get('compatible')

  if isinstance(compat, list):
    compat = compat[0]

  return compat

def get_chosen(root):

  if 'children' in root:
    if 'chosen' in root['children']:
      for k,v in root['children']['chosen']['props'].items():
        chosen[k] = v

  return

def get_phandles(root, name, handles):

  if 'props' in root:
    handle = root['props'].get('phandle')
    enabled = root['props'].get('status')

  if enabled == "disabled":
    return

  if handle != None:
    phandles[handle] = name

  if name != '/':
    name += '/'

  if isinstance(root, dict):
    if root['children']:
      for k,v in root['children'].items():
        get_phandles(v, name + k, handles)

  return

class Loader(yaml.Loader):
    def __init__(self, stream):
        self._root = os.path.realpath(stream.name)
        super(Loader, self).__init__(stream)
        Loader.add_constructor('!include', Loader.include)
        Loader.add_constructor('!import',  Loader.include)

    def include(self, node):
        if isinstance(node, yaml.ScalarNode):
            return self.extractFile(self.construct_scalar(node))

        elif isinstance(node, yaml.SequenceNode):
            result = []
            for filename in self.construct_sequence(node):
                result += self.extractFile(filename)
            return result

        elif isinstance(node, yaml.MappingNode):
            result = {}
            for k,v in self.construct_mapping(node).iteritems():
                result[k] = self.extractFile(v)
            return result

        else:
            print("Error:: unrecognised node type in !include statement")
            raise yaml.constructor.ConstructorError

    def extractFile(self, filename):
        filepath = os.path.join(os.path.dirname(self._root), filename)
        if not os.path.isfile(filepath):
            # we need to look in common directory
            # take path and back up 2 directories and tack on '/common/yaml'
            filepath = os.path.dirname(self._root).split('/')
            filepath = '/'.join(filepath[:-2])
            filepath = os.path.join(filepath + '/common/yaml', filename)
            with open(filepath, 'r') as f:
                return yaml.load(f, Loader)

def insert_defs(node_address, defs, new_defs, new_aliases):
  if node_address in defs:
    if 'aliases' in defs[node_address]:
      defs[node_address]['aliases'].update(new_aliases)
    else:
      defs[node_address]['aliases'] = new_aliases

    defs[node_address].update(new_defs)
  else:
    new_defs['aliases'] = new_aliases
    defs[node_address] = new_defs

  return

def find_node_by_path(nodes, path):
  d = nodes
  for k in path[1:].split('/'):
    d = d['children'][k]

  return d

def compress_nodes(nodes, path):
  if 'props' in nodes:
    status = nodes['props'].get('status')

  if status == "disabled":
    return

  if isinstance(nodes, dict):
    reduced[path] = dict(nodes)
    reduced[path].pop('children', None)
    if path != '/':
      path += '/'
    if nodes['children']:
      for k,v in nodes['children'].items():
        compress_nodes(v, path + k)

  return

def find_parent_irq_node(node_address):
  address = ''

  for comp in node_address.split('/')[1:]:
    address += '/' + comp
    if 'interrupt-parent' in reduced[address]['props']:
      interrupt_parent = reduced[address]['props'].get('interrupt-parent')

  return reduced[phandles[interrupt_parent]]

def extract_interrupts(node_address, yaml, y_key, names, defs, def_label):
  node = reduced[node_address]

  try:
    props = list(node['props'].get(y_key))
  except:
    props = [node['props'].get(y_key)]

  irq_parent = find_parent_irq_node(node_address)

  l_base = def_label.split('/')
  index = 0

  while props:
    prop_def = {}
    prop_alias = {}
    l_idx = [str(index)]

    if y_key == 'interrupts-extended':
      cell_parent = reduced[phandles[props.pop(0)]]
      name = []
    else:
      try:
        name = [names.pop(0).upper()]
      except:
        name = []

      cell_parent = irq_parent

    cell_yaml = yaml[get_compat(cell_parent)]
    l_cell_prefix = [yaml[get_compat(irq_parent)].get('cell_string', []).upper()]

    for i in range(cell_parent['props']['#interrupt-cells']):
      l_cell_name = [cell_yaml['#cells'][i].upper()]
      if l_cell_name == l_cell_prefix:
        l_cell_name = []

      l_fqn = '_'.join(l_base + l_cell_prefix + l_idx + l_cell_name)
      prop_def[l_fqn] = props.pop(0)
      if len(name):
        prop_alias['_'.join(l_base + name + l_cell_prefix)] = l_fqn

    index += 1
    insert_defs(node_address, defs, prop_def, prop_alias)

  return

def extract_reg_prop(node_address, names, defs, def_label, div, post_label):
  node = reduced[node_address]

  props = list(reduced[node_address]['props']['reg'])

  address_cells = reduced['/']['props'].get('#address-cells')
  size_cells = reduced['/']['props'].get('#size-cells')
  address = ''
  for comp in node_address.split('/')[1:]:
    address += '/' + comp
    address_cells = reduced[address]['props'].get('#address-cells', address_cells)
    size_cells = reduced[address]['props'].get('#size-cells', size_cells)

  if post_label is None:
    post_label = "BASE_ADDRESS"

  index = 0
  l_base = def_label.split('/')
  l_addr = [convert_string_to_label(post_label).upper()]
  l_size = ["SIZE"]

  while props:
    prop_def = {}
    prop_alias = {}
    addr = 0
    size = 0
    l_idx = [str(index)]

    try:
      name = [names.pop(0).upper()]
    except:
      name = []

    for x in range(address_cells):
      addr += props.pop(0) << (32 * x)
    for x in range(size_cells):
      size += props.pop(0) << (32 * x)

    l_addr_fqn = '_'.join(l_base + l_addr + l_idx)
    l_size_fqn = '_'.join(l_base + l_size + l_idx)
    prop_def[l_addr_fqn] = hex(addr)
    prop_def[l_size_fqn] = int(size / div)
    if len(name):
      prop_alias['_'.join(l_base + name + l_addr)] = l_addr_fqn
      prop_alias['_'.join(l_base + name + l_size)] = l_size_fqn

    if index == 0:
      prop_alias['_'.join(l_base + l_addr)] = l_addr_fqn
      prop_alias['_'.join(l_base + l_size)] = l_size_fqn

    insert_defs(node_address, defs, prop_def, prop_alias)

    # increment index for definition creation
    index += 1

  return

def extract_cells(node_address, yaml, y_key, names, index, prefix, defs, def_label):
  try:
    props = list(reduced[node_address]['props'].get(y_key))
  except:
    props = [reduced[node_address]['props'].get(y_key)]

  cell_parent = reduced[phandles[props.pop(0)]]

  try:
    cell_yaml = yaml[get_compat(cell_parent)]
  except:
    raise Exception("Could not find yaml description for " + cell_parent['name'])

  try:
    name = names.pop(0).upper()
  except:
    name = []

  l_cell = [str(cell_yaml.get('cell_string',''))]
  l_base = def_label.split('/')
  l_base += prefix
  l_idx = [str(index)]

  prop_def = {}
  prop_alias = {}

  for k in cell_parent['props'].keys():
    if k[0] == '#' and '-cells' in k:
      for i in range(cell_parent['props'].get(k)):
        l_cellname = [str(cell_yaml['#cells'][i]).upper()]
        if l_cell == l_cellname:
          label = l_base + l_cell + l_idx
        else:
          label = l_base + l_cell + l_cellname + l_idx
        label_name = l_base + name + l_cellname
        prop_def['_'.join(label)] = props.pop(0)
        if len(name):
          prop_alias['_'.join(label_name)] = '_'.join(label)

        if index == 0:
          prop_alias['_'.join(label[:-1])] = '_'.join(label)

    insert_defs(node_address, defs, prop_def, prop_alias)

  # recurse if we have anything left
  if len(props):
    extract_cells(node_address, yaml, y_key, names, index + 1, prefix, defs, def_label)

  return

def extract_pinctrl(node_address, yaml, pinconf, names, index, defs, def_label):

  prop_list = []
  if not isinstance(pinconf,list):
    prop_list.append(pinconf)
  else:
    prop_list = list(pinconf)

  def_prefix = def_label.split('_')
  target_node = node_address

  prop_def = {}
  for p in prop_list:
    pin_node_address = phandles[p]
    pin_entry = reduced[pin_node_address]
    parent_address = '/'.join(pin_node_address.split('/')[:-1])
    pin_parent = reduced[parent_address]
    cell_yaml = yaml[get_compat(pin_parent)]
    cell_prefix = cell_yaml.get('cell_string', None)
    post_fix = []

    if cell_prefix != None:
      post_fix.append(cell_prefix)

    for subnode in reduced.keys():
      if pin_node_address in subnode and pin_node_address != subnode:
        # found a subnode underneath the pinmux handle
        node_label = subnode.split('/')[-2:]
        pin_label = def_prefix + post_fix + subnode.split('/')[-2:]

        for i, pin in enumerate(reduced[subnode]['props']['pins']):
          key_label = list(pin_label) + [cell_yaml['#cells'][0]] + [str(i)]
          func_label = key_label[:-2] + [cell_yaml['#cells'][1]] + [str(i)]
          key_label = convert_string_to_label('_'.join(key_label)).upper()
          func_label = convert_string_to_label('_'.join(func_label)).upper()

          prop_def[key_label] = pin
          prop_def[func_label] = reduced[subnode]['props']['function']

  insert_defs(node_address, defs, prop_def, {})

def extract_single(node_address, yaml, prop, key, prefix, defs, def_label):

  prop_def = {}

  if isinstance(prop, list):
    for i, p in enumerate(prop):
      k = convert_string_to_label(key).upper()
      label = def_label + '_' + k
      if isinstance(p, str):
         p = "\"" + p + "\""
      prop_def[label + '_' + str(i)] = p
  else:
      k = convert_string_to_label(key).upper()
      label = def_label + '_' +  k
      if isinstance(prop, str):
         prop = "\"" + prop + "\""
      prop_def[label] = prop

  if node_address in defs:
    defs[node_address].update(prop_def)
  else:
    defs[node_address] = prop_def

  return

def extract_property(node_compat, yaml, node_address, y_key, y_val, names, prefix, defs, label_override):

  node = reduced[node_address]

  if 'base_label' in yaml[node_compat]:
    def_label = yaml[node_compat].get('base_label')
  else:
    def_label = convert_string_to_label(node_compat.upper())
    def_label += '_' + node_address.split('@')[-1].upper()

  if label_override is not None:
    def_label += '_' + label_override

  if y_key == 'reg':
    extract_reg_prop(node_address, names, defs, def_label, 1, y_val.get('label', None))
  elif y_key == 'interrupts' or y_key == 'interupts-extended':
    extract_interrupts(node_address, yaml, y_key, names, defs, def_label)
  elif 'pinctrl-' in y_key:
    p_index = int(y_key.split('-')[1])
    extract_pinctrl(node_address, yaml, reduced[node_address]['props'][y_key],
                    names[p_index], p_index, defs, def_label)
  elif 'clocks' in y_key:
    extract_cells(node_address, yaml, y_key,
                  names, 0, prefix, defs, def_label)
  else:
    extract_single(node_address, yaml,
                   reduced[node_address]['props'][y_key], y_key,
                   prefix, defs, def_label)

  return

def extract_node_include_info(reduced, root_node_address, sub_node_address,
                              yaml, defs, structs, y_sub):
  node = reduced[sub_node_address]
  node_compat = get_compat(reduced[root_node_address])
  label_override = None

  if not node_compat in yaml.keys():
    return {}, {}

  if y_sub is None:
    y_node = yaml[node_compat]
  else:
    y_node = y_sub

  if yaml[node_compat].get('use-property-label', False):
    for yp in y_node['properties']:
      if yp.get('label') is not None:
        if node['props'].get('label') is not None:
          label_override = convert_string_to_label(node['props']['label']).upper()
          break

  # check to see if we need to process the properties
  for yp in y_node['properties']:
    for k,v in yp.items():
      if 'properties' in v:
        for c in reduced:
          if root_node_address + '/' in c:
            extract_node_include_info(reduced, root_node_address, c, yaml, defs, structs, v)
      if 'generation' in v:
        if v['generation'] == 'define':
          label = v.get('define_string')
          storage = defs
        else:
          label = v.get('structures_string')
          storage = structs

        prefix = []
        if v.get('use-name-prefix') != None:
          prefix = [convert_string_to_label(k.upper())]

        for c in node['props'].keys():
          if c.endswith("-names"):
            pass

          if re.match(k + '$', c):

            if 'pinctrl-' in c:
              names = node['props'].get('pinctrl-names', [])
            else:
              names = node['props'].get(c[:-1] + '-names', [])
              if not names:
                names = node['props'].get(c + '-names', [])

            if not isinstance(names, list):
              names = [names]

            extract_property(node_compat, yaml, sub_node_address, c, v, names, prefix, defs, label_override)

  return

def yaml_collapse(yaml_list):
  collapsed = dict(yaml_list)

  for k,v in collapsed.items():
    props = set()
    if 'properties' in v:
       for entry in v['properties']:
         for key in entry:
           props.add(key)

    if 'inherits' in v:
      for inherited in v['inherits']:
        for prop in inherited['properties']:
          for key in prop:
            if key not in props:
              v['properties'].append(prop)
      v.pop('inherits')

  return collapsed


def print_key_value(k, v, tabstop):
  label = "#define " + k

  # calculate the name's tabs
  if len(label) % 8:
    tabs = (len(label) + 7)  >> 3
  else:
    tabs = (len(label) >> 3) + 1

  sys.stdout.write(label)
  for i in range(0, tabstop - tabs + 1):
    sys.stdout.write('\t')
  sys.stdout.write(str(v))
  sys.stdout.write("\n")

  return

def generate_keyvalue_file(defs, args):
    compatible = reduced['/']['props']['compatible'][0]

    node_keys = sorted(defs.keys())
    for node in node_keys:
       sys.stdout.write('# ' + node.split('/')[-1] )
       sys.stdout.write("\n")

       prop_keys = sorted(defs[node].keys())
       for prop in prop_keys:
         if prop == 'aliases':
           for entry in sorted(defs[node][prop]):
             a = defs[node][prop].get(entry)
             sys.stdout.write("%s=%s\n" %(entry, defs[node].get(a)))
         else:
           sys.stdout.write("%s=%s\n" %(prop,defs[node].get(prop)))

       sys.stdout.write("\n")

def generate_include_file(defs, args):
    compatible = reduced['/']['props']['compatible'][0]

    sys.stdout.write("/**************************************************\n")
    sys.stdout.write(" * Generated include file for " + compatible)
    sys.stdout.write("\n")
    sys.stdout.write(" *               DO NOT MODIFY\n");
    sys.stdout.write(" */\n")
    sys.stdout.write("\n")
    sys.stdout.write("#ifndef _DEVICE_TREE_BOARD_H" + "\n");
    sys.stdout.write("#define _DEVICE_TREE_BOARD_H" + "\n");
    sys.stdout.write("\n")

    node_keys = sorted(defs.keys())
    for node in node_keys:
       sys.stdout.write('/* ' + node.split('/')[-1] + ' */')
       sys.stdout.write("\n")

       maxlength = max(len(s + '#define ') for s in defs[node].keys())
       if maxlength % 8:
           maxtabstop = (maxlength + 7) >> 3
       else:
         maxtabstop = (maxlength >> 3) + 1

       if (maxtabstop * 8 - maxlength) <= 2:
         maxtabstop += 1

       prop_keys = sorted(defs[node].keys())
       for prop in prop_keys:
         if prop == 'aliases':
           for entry in sorted(defs[node][prop]):
             a = defs[node][prop].get(entry)
             print_key_value(entry, a, maxtabstop)
         else:
           print_key_value(prop, defs[node].get(prop), maxtabstop)

       sys.stdout.write("\n")

    if args.fixup and os.path.exists(args.fixup):
        sys.stdout.write("\n")
        sys.stdout.write("/* Following definitions fixup the generated include */\n")
        try:
            with open(args.fixup, "r") as fd:
                for line in fd.readlines():
                    sys.stdout.write(line)
                sys.stdout.write("\n")
        except:
            raise Exception("Input file " + os.path.abspath(args.fixup) + " does not exist.")

    sys.stdout.write("#endif\n")

def lookup_defs(defs, node, key):
    if node not in defs:
        return None

    if key in defs[node]['aliases']:
        key = defs[node]['aliases'][key]

    return defs[node].get(key, None)


def parse_arguments():

  parser = argparse.ArgumentParser(description = __doc__,
                                     formatter_class = argparse.RawDescriptionHelpFormatter)

  parser.add_argument("-d", "--dts", help="DTS file")
  parser.add_argument("-y", "--yaml", help="YAML file")
  parser.add_argument("-f", "--fixup", help="Fixup file")
  parser.add_argument("-k", "--keyvalue", action="store_true",
          help="Generate file to be included by the build system")

  return parser.parse_args()

def main():
  args = parse_arguments()
  if not args.dts or not args.yaml:
    print('Usage: %s -d filename.dts -y path_to_yaml' % sys.argv[0])
    return 1

  try:
    with open(args.dts, "r") as fd:
      d = parse_file(fd)
  except:
     raise Exception("Input file " + os.path.abspath(args.dts) + " does not exist.")

  # compress list to nodes w/ paths, add interrupt parent
  compress_nodes(d['/'], '/')

  # build up useful lists
  compatibles = get_all_compatibles(d['/'], '/', {})
  get_phandles(d['/'], '/', {})
  get_aliases(d['/'])
  get_chosen(d['/'])

  # find unique set of compatibles across all active nodes
  s = set()
  for k,v in compatibles.items():
    if isinstance(v,list):
      for item in v:
        s.add(item)
    else:
      s.add(v)

  # scan YAML files and find the ones we are interested in
  yaml_files = []
  for filename in listdir(args.yaml):
    if re.match('.*\.yaml\Z', filename):
      yaml_files.append(os.path.realpath(args.yaml + '/' + filename))

  # scan common YAML files and find the ones we are interested in
  zephyrbase = os.environ.get('ZEPHYR_BASE')
  if zephyrbase is not None:
    for filename in listdir(zephyrbase + '/dts/common/yaml'):
      if re.match('.*\.yaml\Z', filename):
        yaml_files.append(os.path.realpath(zephyrbase + '/dts/common/yaml/' + filename))

  yaml_list = {}
  file_load_list = set()
  for file in yaml_files:
    for line in open(file, 'r'):
      if re.search('^\s+constraint:*', line):
        c = line.split(':')[1].strip()
        c = c.strip('"')
        if c in s:
          if not file in file_load_list:
            file_load_list.add(file)
            with open(file, 'r') as yf:
              yaml_list[c] = yaml.load(yf, Loader)

  if yaml_list == {}:
    raise Exception("Missing YAML information.  Check YAML sources")

  # collapse the yaml inherited information
  yaml_list = yaml_collapse(yaml_list)

  defs = {}
  structs = {}
  for k, v in reduced.items():
    node_compat = get_compat(v)
    if node_compat != None and node_compat in yaml_list:
      extract_node_include_info(reduced, k, k, yaml_list, defs, structs, None)

  if defs == {}:
    raise Exception("No information parsed from dts file.")

  if 'zephyr,flash' in chosen:
    extract_reg_prop(chosen['zephyr,flash'], None, defs, "CONFIG_FLASH", 1024, None)
  else:
    # We will add address and size of 0 for systems with no flash controller
    # This is what they already do in the Kconfig options anyway
    defs['dummy-flash'] =  { 'CONFIG_FLASH_BASE_ADDRESS': 0, 'CONFIG_FLASH_SIZE': 0 }

  if 'zephyr,sram' in chosen:
    extract_reg_prop(chosen['zephyr,sram'], None, defs, "CONFIG_SRAM", 1024, None)

  # only compute the load offset if a code partition exists and it is not the
  # same as the flash base address
  load_defs = {}
  if 'zephyr,code-partition' in chosen and \
     'zephyr,flash' in chosen and \
     reduced[chosen['zephyr,flash']] is not \
              reduced[chosen['zephyr,code-partition']]:
    flash_base = lookup_defs(defs, chosen['zephyr,flash'],
                              'CONFIG_FLASH_BASE_ADDRESS')
    part_defs = {}
    extract_reg_prop(chosen['zephyr,code-partition'], None, part_defs,
                     "PARTITION", 1, 'offset')
    part_base = lookup_defs(part_defs, chosen['zephyr,code-partition'],
                     'PARTITION_OFFSET')
    load_defs['CONFIG_FLASH_LOAD_OFFSET'] = part_base
    load_defs['CONFIG_FLASH_LOAD_SIZE'] = \
            lookup_defs(part_defs, chosen['zephyr,code-partition'],
                        'PARTITION_SIZE')
  else:
    load_defs['CONFIG_FLASH_LOAD_OFFSET'] = 0
    load_defs['CONFIG_FLASH_LOAD_SIZE'] = 0

  insert_defs(chosen['zephyr,flash'], defs, load_defs, {})

  # generate include file
  if args.keyvalue:
    generate_keyvalue_file(defs, args)
  else:
    generate_include_file(defs, args)

if __name__ == '__main__':
    main()
